// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#ifdef _WIN32
#define NOMINMAX
#include <WS2tcpip.h>
#include <Windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
typedef long ssize_t;
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

#include "socketconnection.h"
#include "task.h"
#include <sys/types.h>
#ifdef USE_GNUTLS
#include <gnutls/x509.h>
#endif

SocketConnection::
SocketConnection(const std::string &label, Task *owner,
                 const std::string &hostname, uint16_t port,
                 uint16_t iptype, struct addrinfo *local_addr) :
    Socket(label, owner, hostname, port) {
    peer_ip = hostname;
    peer_port = port;
    local_ip = local_addr;
    if (local_ip)
        prefer_ip_type = (local_ip->ai_family == AF_INET6 ? 6 : 4);
    else
        prefer_ip_type = iptype;

}

#ifdef USE_THREADS
thread_local
#endif
uint64_t SocketConnection::tot_bytes_sent = 0;
#ifdef USE_THREADS
thread_local
#endif
uint64_t SocketConnection::tot_bytes_received = 0;

SocketConnection::SocketConnection(const std::string &label, Task *owner,
                                   int fd, const char *ip, uint16_t port) :
    Socket(label, owner, fd) {
    peer_ip = ip;
    peer_port = port;
}

void SocketConnection::closedByPeer() {
    log() << "connection closed by peer";
}

PollState SocketConnection::doRead(int fd) {
    ssize_t n;
#ifdef USE_GNUTLS
    if (is_tls()) {
        n = gnutls_record_recv(session, socket_buffer, sizeof socket_buffer);
        if (n < 0) {
            if (gnutls_error_is_fatal(static_cast<int>(n))) {
                // Probably client terminated the connection abruptly
                dbg_log() << "TLS error on socket " << fd;
                return PollState::CLOSE;
            }
            //warn_log() << "TLS recv interrupt on socket " << fd;
            return state();
        }
    } else
#endif
    n = recv(fd, socket_buffer, sizeof socket_buffer, 0);
    if (debugging)
        log() << "socket " << socket() << " doRead " << n;
    if (n == 0) {
        log() << "socket closed by peer " << fd;
        return PollState::CLOSE;
    } else if (n < 0) {
        if (isTempError()) {
            warn_log() << "recv interrupt on socket " << fd;
            return state();
        } else {
            errno_log() << "recv error on socket " << fd;
            return PollState::CLOSE;
        }
    }
    if (debugging && n < 1000)
        log() << "socket " << socket() << "doRead <"
              << std::string(socket_buffer, static_cast<size_t>(n));
#ifdef USE_GNUTLS
    // For SSL sockets, it's the tls_pull method that counts the actual number
    // of bytes fetched from the network
    if (!is_tls())
#endif
    {
        tot_bytes_received += static_cast<uint64_t>(n);
        owner()->notifyBytesReceived(static_cast<uint64_t>(n));
    }

    // Now let the subclass take care of what we read:
    if (state() == PollState::READ || state() == PollState::READ_WRITE) {
        PollState ret = readData(socket_buffer, static_cast<size_t>(n));
        // If there's more to read, do it immediately instead of after
        // next select call:
        if (ret == PollState::READ &&
            static_cast<size_t>(n) == sizeof socket_buffer)
            return doRead(fd);
        return ret;
    }
    return unexpectedData(socket_buffer, static_cast<size_t>(n));
}

// Must not be called more than once on the same object!
bool SocketConnection::asyncConnect() {
    if (socket() >= 0) {
        log() << "internal error, connect called twice on socket" << socket();
        return false;
    }

    struct addrinfo *addr = getAddressInfo(prefer_ip_type);
    if (!addr)
        return false;

    createNonBlockingSocket(addr, local_ip);
    if (socket() < 0)
        return false;

    return true;
}

PollState SocketConnection::unexpectedData(char *buf, size_t len) {
    err_log() << "unexpected data arrived; will close connection: "
              << std::string(buf, std::min(len, static_cast<size_t>(30)));
    return PollState::CLOSE;
}

size_t SocketConnection::sendData(const char *buf, size_t len) {
    ssize_t n;
#ifdef USE_GNUTLS
    if (is_tls()) {
        n = gnutls_record_send(session, buf, len);
        if (n < 0) {
            if (gnutls_error_is_fatal(static_cast<int>(n))) {
                errno_log() << "TLS write error";
                closeMe();
            } else {
                if (!tls_send_pending) {
                    warn_log() << "Socket " << socket()
                               << " TLS write failure: "
                               << gnutls_strerror(static_cast<int>(n));
                    tls_send_pending = true;
                }
            }
            return 0;
        }
        // To reenable the GNUTLS_E_AGAIN warning, do this:
        // tls_send_pending = false;
        if (debugging && n < 1000)
            log() << "socket " << socket() << "sendData <"
                  << std::string(buf, static_cast<size_t>(n)) << ">";
        // For SSL sockets, it's the tls_push method that counts the actual
        // number of bytes sent over the network
        return static_cast<size_t>(n);
    }
#endif
#ifdef __APPLE__
    n = send(socket(), buf, len, 0);
#elif defined(_WIN32)
    n = send(socket(), buf, len, 0);
#else
    n = send(socket(), buf, len, MSG_NOSIGNAL);
#endif
    if (debugging)
        log() << "socket " << socket() << "sendData " << n << " of " << len;
    if (n < 0) {
        if (!isTempError()) {
            errno_log() << "cannot write";
            closeMe();
        }
        return 0;
    }
    if (debugging && n < 1000)
        log() << "socket " << socket() << "sendData <"
              << std::string(buf, static_cast<size_t>(n)) << ">";
    // Global count
    tot_bytes_sent += static_cast<uint64_t>(n);
    // Per task count
    owner()->notifyBytesSent(static_cast<uint64_t>(n));
    return static_cast<size_t>(n);
}

#ifdef USE_WEBROOT
size_t SocketConnection::sendFileData(int fd, size_t len) {
    static char buf[50000];
    if (len > sizeof buf)
        len = sizeof buf;
    ssize_t n = read(fd, buf, len);
    log() << "Read " << n << " bytes from file";
    if (n <= 0) {
        errno_log() << "cannot read from file";
        closeMe();
        return 0;
    }
    return sendData(buf, static_cast<size_t>(n));
}
#endif

void SocketConnection::asyncSendData(const char *buf, size_t len) {
    if (to_send.empty()) {
        size_t sent = sendData(buf, len);
        if (sent == len)
            return;
        to_send = std::string(buf+sent, len-sent);
    } else {
        to_send.append(buf, len);
    }
}

PollState SocketConnection::tellOwner(const std::string &msg) {
    return owner()->msgFromConnection(this, msg);
}

#ifdef USE_GNUTLS

bool SocketConnection::
init_tls_client(gnutls_certificate_credentials_t &x509_cred, bool verify_cert) {
    dbg_log() << "Enable TLS on socket " << socket();

    if (gnutls_init(&session, GNUTLS_CLIENT | GNUTLS_NONBLOCK) < 0)
        return false;
    setSessionInitialized();
    if (gnutls_set_default_priority(session) < 0)
        return false;
    if (gnutls_server_name_set(session, GNUTLS_NAME_DNS, peer_ip.c_str(),
                               peer_ip.size()) < 0)
        return false;

    if (gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
                               x509_cred) < 0)
        return false;

    if (verify_cert) {
        log() << "verify cert";
        gnutls_session_set_verify_cert(session, peer_ip.c_str(), 0);
    }

    gnutls_certificate_set_verify_flags (x509_cred,
                                         GNUTLS_VERIFY_ALLOW_BROKEN);

    gnutls_handshake_set_timeout(session,
                                 GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);

    // gnutls_transport_set_int(session, socket());
    // We use custom send/recv functions in order to be able to count the
    // number of bytes sent and received:
    {
        gnutls_transport_set_ptr(session,
                                 static_cast<gnutls_transport_ptr_t>(this));
        gnutls_transport_set_push_function(session, tls_push_static);
        gnutls_transport_set_pull_function(session, tls_pull_static);
    }
    return true;
}

ssize_t SocketConnection::tls_pull(void *buf, size_t len) {
    ssize_t n = recv(socket(), buf, len, 0);
    //dbg_log() << "tls_pull " << n << " bytes of " << len;
    if (n > 0) {
        owner()->notifyBytesReceived(static_cast<uint64_t>(n));
        tot_bytes_received += static_cast<uint64_t>(n);
    }
    return n;
}

ssize_t SocketConnection::tls_push(const void *buf, size_t len) {
#ifdef __APPLE__
    ssize_t n = send(socket(), buf, len, 0);
#elif defined(_WIN32)
    ssize_t n = send(socket(), buf, len, 0);
#else
    ssize_t n = send(socket(), buf, len, MSG_NOSIGNAL);
#endif
    //dbg_log() << "tls_push " << n << " bytes of " << len;
    if (n > 0) {
        owner()->notifyBytesSent(static_cast<uint64_t>(n));
        tot_bytes_sent += static_cast<uint64_t>(n);
    }
    return n;
}

bool SocketConnection::
init_tls_server(gnutls_certificate_credentials_t &x509_cred,
                gnutls_priority_t &priority_cache) {
    dbg_log() << "Enable TLS on incoming socket " << socket();

    if (gnutls_init(&session, GNUTLS_SERVER | GNUTLS_NONBLOCK) < 0) {
        err_log() << "Cannot initialise TLS session";
        return false;
    }
    setSessionInitialized();

    if (gnutls_priority_set(session, priority_cache) < 0 ||
        gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE,
                               x509_cred) < 0)
        return false;

    gnutls_certificate_server_set_request(session,
                                          GNUTLS_CERT_IGNORE);
    gnutls_handshake_set_timeout(session,
                                 GNUTLS_DEFAULT_HANDSHAKE_TIMEOUT);
    // gnutls_transport_set_int(session, socket());
    // We use custom send/recv functions in order to be able to count the
    // number of bytes sent and received:
    {
        gnutls_transport_set_ptr(session,
                                 static_cast<gnutls_transport_ptr_t>(this));
        gnutls_transport_set_push_function(session, tls_push_static);
        gnutls_transport_set_pull_function(session, tls_pull_static);
    }

    return true;
}

#endif
