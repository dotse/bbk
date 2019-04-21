// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

// This is a slave to the Engine class. You can't use it directly,
// only through its subclasses, SocketConnection or ServerSocket.

#pragma once

#ifdef _WIN32
#include <winsock2.h>
#endif

#include "pollstate.h"
#include "logger.h"

class Task;

class Socket : public Logger {
    friend class Engine;
public:
    Socket(const std::string &label, Task *owner,
           const std::string &hostname, uint16_t port);

    Socket(const std::string &label, Task *owner, int fd);

    Task *owner() const {
        return _owner;
    }

    std::string hostname() const {
        return _hostname;
    }

    uint16_t port() const {
        return _port;
    }

    PollState state() const {
        return _state;
    }

#ifndef _WIN32
    // Return the peer socket descriptor if this is a Unix Domain socket
    // connection. If not, return 0. May be used in another thread or process.
    int getUnixDomainPeer() const {
        return unix_domain_peer;
    }
#endif

    virtual ~Socket();

    // By default, if we have a keepalive (cached) connection to the same
    // host and host and port, it will be used instead of creating a
    // new connection. Override this method to disable keepalive
    // (returning empty string) or to use only a special type of
    // cached connection (returning a label for that special type).
    virtual std::string cacheLabel() {
        return _hostname + std::to_string(_port);
    }

    // Returns a positive number that is unique to this connection if it is
    // active, otherwise -1.
    int id() const {
        return _socket;
    }

    virtual void setOwner(Task *t) {
        _owner = t;
    }

    // Call this to have the socket removed automatically before a given number
    // of seconds. Note: the network engine might remove the socket 1-2 seconds
    // before the timeout, so adjust the timeout value accordingly!
    void setExpiry(double s) {
        expiry = timeAfter(s);
    }

    bool hasExpired(const TimePoint &when) const {
        return (expiry < when);
    }
    const char *localIp() const {
        const char *ip = getIp(socket(), nullptr, false);
        return ip;
    }
    // Return IP address of connected socket in static buffer.
    // Returns the local IP address if peer==false, otherwise the peer IP.
    static const char *getIp(int fd, uint16_t *port = nullptr,
                             bool peer = true);
    static const char *getIp(struct sockaddr *address, uint16_t *port=nullptr);
    static const char *getIp(struct addrinfo *address, uint16_t *port=nullptr);
    struct addrinfo *getAddressInfo(uint16_t iptype = 0);
protected:
    static bool isTempError() {
#ifdef _WIN32
        if (WSAGetLastError() == WSAEWOULDBLOCK ||
            WSAGetLastError() == WSAEINPROGRESS ||
            WSAGetLastError() == WSAENOTCONN ||
            !WSAGetLastError())
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINPROGRESS
            || errno == EINTR || !errno)
#endif
            return true;
        else
            return false;
    }

    virtual bool wantToSend() {
        return false;
    }

    // This will be called regularly on READ_BLOCKED sockets to check if the
    // block can be lifted. If your subclass ever returns READ_BLOCKED,
    // it should override this method to return the new state when the block
    // should be removed.
    virtual PollState checkReadBlock() {
        return PollState::READ_BLOCKED;
    }

    void setWantToSend() {
        if (_state != PollState::CLOSE)
            _state = PollState::READ_WRITE;
    }

    int socket() const {
        return _socket;
    }

    static int closeSocket(int fd);

    void closeMe() {
        _state = PollState::CLOSE;
    }
    void createNonBlockingSocket(struct addrinfo *addressEntry,
                                 struct addrinfo *localAddr);
    bool setNonBlocking(int fd);
    static bool socketInError(int fd);
    bool inError() const {
        if (!socketInError(_socket))
            return false;
        errno_log() << "failed socket " << _socket;
        return true;
    }
private:
    void setState(PollState state) {
        if (_state != PollState::CLOSE)
            _state = state;
    }

    // Return false on failure:
    bool createServerSocket();

    static void clearCache();

    void killMe() {
        _state = PollState::KILL;
    }

    // Internal identifier used as key in dns_cache:
    std::string _peer_label;

    void setSocket(int fd) {
        _socket = fd;
    }

    Socket(const Socket &);
    Task *_owner;
    int _socket;
#ifndef _WIN32
    // If this is a Unix Domain socket, the peer socket descriptor will be
    // stored here:
    int unix_domain_peer = 0;
#endif
    std::string _hostname;
    uint16_t _port;
    PollState _state;
    TimePoint expiry = timeMax();
};
