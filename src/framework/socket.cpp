// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#ifdef _WIN32
#include <ws2tcpip.h>
#include <time.h>
#define NOMINMAX
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <net/if.h>
#endif
#ifdef __linux
#include <netinet/tcp.h>
#endif

#include <fcntl.h>
#include <sys/types.h>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <cerrno>

#include <string>
#include <iomanip>
#include <sstream>
#include <map>

#include "socket.h"
#include "task.h"

Socket::Socket(const std::string &label, Task *owner,
               const std::string &hostname, uint16_t port) :
    Logger(label),
    _owner(owner),
    _hostname(hostname),
    _port(port),
    _state(PollState::NONE)
{
#ifndef _WIN32
    if (!port && hostname == "UnixDomain") {
        int pair_sd[2];
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, pair_sd) < 0) {
            errno_log() << "cannot create socket pair";
            _socket = 0;
        } else {
            _socket = pair_sd[0];
            unix_domain_peer = pair_sd[1];
            fcntl(pair_sd[0], F_SETFL, O_NONBLOCK|O_CLOEXEC);
            fcntl(pair_sd[1], F_SETFL, O_NONBLOCK);
        }
        return;
    }
#endif
    _socket = -1;
    _peer_label = _hostname + ":" + std::to_string(_port);
}

// TODO: take initial state as a parameter, default PollState::READ.
Socket::Socket(const std::string &label, Task *owner, int fd) :
    Logger(label),
    _owner(owner),
    _socket(fd),
    _hostname(""),
    _port(0),
    _state(PollState::READ)
{
}

Socket::~Socket() {
    if (_socket >= 0) {
        closeSocket(_socket);
        log() << "closed socket " << _socket;
    }
}

namespace {
#ifdef USE_THREADS
    thread_local
#endif
    std::map<std::string, struct addrinfo *> dns_cache;
}

void Socket::clearCache() {
    for (auto p : dns_cache)
        freeaddrinfo(p.second);
    dns_cache.clear();
}

struct addrinfo *Socket::getAddressInfo(uint16_t iptype) {
    auto it = dns_cache.find(_peer_label);
    if (it == dns_cache.end()) {
        struct addrinfo hints, *addressInfo;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_flags = AI_ADDRCONFIG;
        if (_hostname.find_first_not_of("1234567890.:") == std::string::npos) {
            hints.ai_flags |= AI_NUMERICHOST;
            log() << "numeric address " << _hostname;
        } else
            log() << "dns lookup " << _hostname;

        const char *hostaddr;
        if (_hostname.empty()) {
            hints.ai_family = AF_INET6;
            hints.ai_flags |= AI_PASSIVE;
            log() << "wildcard address *:" << _port;
            hostaddr = nullptr;
        } else if (_hostname.find_first_not_of("1234567890.:") ==
                   std::string::npos) {
            hints.ai_flags |= AI_NUMERICHOST;
            log() << "numeric address " << _hostname;
            hostaddr = _hostname.c_str();
        } else {
            log() << "dns lookup " << _hostname;
            hostaddr = _hostname.c_str();
        }

        int res = getaddrinfo(hostaddr, std::to_string(_port).c_str(),
                              &hints, &addressInfo);
        if (res != 0) {
            err_log() << "lookup failed: " << gai_strerror(res);
            return nullptr;
        } else if (!addressInfo) {
            err_log() << "no valid address found";
            return nullptr;
        }
        if (!(hints.ai_flags & AI_NUMERICHOST)) {
            char ip[INET6_ADDRSTRLEN];
            struct sockaddr *addr = addressInfo->ai_addr;
            if (addressInfo->ai_family == AF_INET) {
                struct sockaddr_in *s = reinterpret_cast<sockaddr_in *>(addr);
                inet_ntop(AF_INET, &s->sin_addr, ip, sizeof ip);
            } else {
                struct sockaddr_in6 *s = reinterpret_cast<sockaddr_in6 *>(addr);
                inet_ntop(AF_INET6, &s->sin6_addr, ip, sizeof ip);
            }
            log() << "lookup done: " << ip;
        }

        auto p2 = dns_cache.insert(std::make_pair(_peer_label, addressInfo));
        it = p2.first;
    }
    if (iptype) {
        int fam = (iptype == 6) ? AF_INET6 : AF_INET;
        struct addrinfo *ai = it->second;
        while (ai) {
            if (ai->ai_family == fam)
                return ai;
            ai = ai->ai_next;
        }
    }
    return it->second;
}

void Socket::createNonBlockingSocket(struct addrinfo *addressEntry,
                                     struct addrinfo *localAddr) {
    if (_socket >= 0) {
        err_log() << "socket already exists";
        return;
    }
    int fd = ::socket(addressEntry->ai_family, addressEntry->ai_socktype,
                      addressEntry->ai_protocol);
    if (fd == -1) {
        errno_log() << "cannot create socket";
        return;
    }
    if (!setNonBlocking(fd)) {
        closeSocket(fd);
        return;
    }

    if (localAddr && bind(fd, localAddr->ai_addr, localAddr->ai_addrlen) != 0) {
        errno_log() << "cannot bind to local address";
        return;
    }

    int res = connect(fd, addressEntry->ai_addr, addressEntry->ai_addrlen);
    if (res == -1 && !isTempError()) {
        errno_log() << "connect error";
        closeSocket(fd);
        return;
    }

    // All good, let's keep the socket:
    _socket = fd;
    _state = PollState::CONNECTING;
}

int Socket::closeSocket(int fd) {
#ifdef _WIN32
     return closesocket(fd);
#else
     return close(fd);
#endif
}

bool Socket::socketInError(int fd) {
    int res;
    socklen_t res_len = sizeof(res);
#ifdef _WIN32
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&res, &res_len) < 0)
#else
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &res, &res_len) < 0)
#endif
        return true;
    if (!res)
        return false;
    errno = res;
    return !isTempError();
}

bool Socket::setNonBlocking(int fd) {
#ifdef __APPLE__
    // SO_NOSIGPIPE only for OS X
    int value = 1;
    int status = setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE,
                            &value, sizeof(value));
    if (status != 0) {
        errno_log() << "cannot set SO_NOSIGPIPE";
    }
#endif

#ifdef _WIN32
    u_long enabledParameter = 1;
    int nonBlockingResult = ioctlsocket(fd, FIONBIO, &enabledParameter);

    if (nonBlockingResult == -1) {
        errno_log() << "cannot set socket non-blocking";
        closesocket(fd);
        return false;
    }
#else
    int nonBlockingResult = fcntl(fd, F_SETFL, O_NONBLOCK);
    if (nonBlockingResult == -1) {
        errno_log() << "cannot set socket non-blocking";
        close(fd);
        return false;
    }
#endif
#ifdef __linux
    int flag = 1;
    int result = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                            reinterpret_cast<char *>(&flag), sizeof(int));
    if (result < 0)
        errno_log() << "cannot set TCP_NODELAY";
#endif
    return true;
}

const char *Socket::getIp(struct sockaddr *address, uint16_t *port) {
#ifdef USE_THREADS
    thread_local
#endif
    static char client_ip[INET6_ADDRSTRLEN];
    if (address->sa_family == AF_INET) {
        struct sockaddr_in *s = reinterpret_cast<sockaddr_in *>(address);
        inet_ntop(AF_INET, &s->sin_addr, client_ip, INET6_ADDRSTRLEN);
        if (port)
            *port = ntohs(s->sin_port);
    } else {
        struct sockaddr_in6 *s = reinterpret_cast<sockaddr_in6 *>(address);
        inet_ntop(AF_INET6, &s->sin6_addr, client_ip, INET6_ADDRSTRLEN);
        if (port)
            *port = ntohs(s->sin6_port);
    }
    if (strncmp(client_ip, "::ffff:", 7) == 0)
        return client_ip+7;
    else
        return client_ip;
}

const char *Socket::getIp(struct addrinfo *address, uint16_t *port) {
    return getIp(address->ai_addr, port);
}

const char *Socket::getIp(int fd, uint16_t *port, bool peer) {
#ifdef USE_THREADS
    thread_local
#endif
    static char client_ip[INET6_ADDRSTRLEN];
    static const char *no_ip = "unknown IP";

    struct sockaddr_storage address;
    memset(&address, 0, sizeof address);
    socklen_t addrlen = sizeof(address);

    int ret = peer ?
        getpeername(fd, reinterpret_cast<sockaddr *>(&address), &addrlen) :
        getsockname(fd, reinterpret_cast<sockaddr *>(&address), &addrlen);

    if (ret < 0) {
        return no_ip;
    } else {
        if (address.ss_family == AF_INET) {
            struct sockaddr_in *s = reinterpret_cast<sockaddr_in *>(&address);
            inet_ntop(AF_INET, &s->sin_addr, client_ip, INET6_ADDRSTRLEN);
            if (port)
                *port = ntohs(s->sin_port);
        } else {
            struct sockaddr_in6 *s = reinterpret_cast<sockaddr_in6 *>(&address);
            inet_ntop(AF_INET6, &s->sin6_addr, client_ip, INET6_ADDRSTRLEN);
            if (port)
                *port = ntohs(s->sin6_port);
        }
        if (strncmp(client_ip, "::ffff:", 7) == 0)
            return client_ip+7;
        else
            return client_ip;
    }
}

bool Socket::createServerSocket() {
    std::string ip = _hostname;

    if (_socket >= 0)
        return false; // Already in use!!

    log() << "Listen on " << port() << " ip " << ip;

    struct addrinfo *addr = getAddressInfo();
    if (!addr)
        return false;

    int fd = ::socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
    if (fd < 0) {
        errno_log() << "cannot create listen socket";
        return false;
    }
#ifndef _WIN32
    int reuse = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                   sizeof(reuse)) < 0) {
        errno_log() << "cannot reuse listen socket";
        return false;
    }
#endif

    if (bind(fd, addr->ai_addr, addr->ai_addrlen) != 0) {
        errno_log() << "cannot bind listen socket";
        return false;
    }

    if (listen(fd, 20) != 0) {
        errno_log() << "cannot listen";
        return false;
    }

    // Socket will be -1, and state will be UNDEFINED, unless we get here:
    _socket = fd;
    _state = PollState::READ;

    // Check port number
    struct sockaddr_in6 address;
    socklen_t len = sizeof(address);
    if (getsockname(fd, reinterpret_cast<sockaddr *>(&address), &len) == -1)
        errno_log() << "getsockname failed";
    else {
        _port = ntohs(address.sin6_port);
        log() << "server socket " << fd << " listening on port " << _port;
    }

    return true;
}
