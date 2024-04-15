// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#ifdef _WIN32
#include <winsock2.h>
#endif

#include "pollstate.h"
#include "logger.h"

class Task;

/// \brief
/// This is a slave to the Engine class. You can't use it directly,
/// only through its subclasses, SocketConnection or ServerSocket.
class Socket : public Logger {
    friend class Engine;
public:
    Socket(const std::string &label, Task *owner,
           const std::string &hostname, uint16_t port);

    Socket(const std::string &label, Task *owner, int fd);

    /// Return task owning the socket.
    Task *owner() const {
        return _owner;
    }

    /// Return name of the host to which the socket is supposed to connect.
    std::string hostname() const {
        return _hostname;
    }

    /// Return port number to which the socket is supposed to connect.
    uint16_t port() const {
        return _port;
    }

    /// \brief
    /// Return current socket state.
    PollState state() const {
        return _state;
    }

#ifndef _WIN32
    /// \brief
    /// Return the peer socket descriptor.
    ///
    /// If this is a Unix Domain socket, return the peer socket descriptor.
    /// If not, return 0.
    ///
    /// May be used in another thread or process.
    int getUnixDomainPeer() const {
        return unix_domain_peer;
    }
#endif

    virtual ~Socket();

    /// \brief
    /// Return the socket's cache group, or an empty string.
    ///
    /// By default, if we have a keepalive (cached) connection to the same
    /// host and port, it will be used instead of creating a
    /// new connection. Override this method to disable keepalive
    /// (returning empty string) or to use only a special type of
    /// cached connection (returning a label for that special type).
    virtual std::string cacheLabel() {
        return _hostname + std::to_string(_port);
    }

    /// \brief
    /// Return unique connection ID if connected.
    ///
    /// Return a positive number that is unique to this connection if it is
    /// active, otherwise -1.
    int id() const {
        return _socket;
    }

    /// Set the given task as owner of the socket.
    virtual void setOwner(Task *t) {
        _owner = t;
    }

    /// \brief
    /// Set a time to live for the socket.
    ///
    /// Call this to have the socket removed automatically before a given number
    /// of seconds. Note: the network engine might remove the socket 1-2 seconds
    /// before the timeout, so adjust the timeout value accordingly!
    void setExpiry(double s) {
        expiry = timeAfter(s);
    }

    /// Return true if the given TimePoint is after the socket's expiry.
    bool hasExpired(const TimePoint &when) const {
        return (expiry < when);
    }

    /// Return local IP address in static buffer.
    const char *localIp() const {
        const char *ip = getIp(socket(), nullptr, false);
        return ip;
    }

    /// \brief
    /// Return IP address of connected socket in static buffer.
    ///
    /// Return the local IP address if peer==false, otherwise the peer IP.
    static const char *getIp(int fd, uint16_t *port = nullptr,
                             bool peer = true);

    /// Return IP address in static buffer.
    static const char *getIp(struct sockaddr *address, uint16_t *port=nullptr);

    /// Return IP address in static buffer.
    static const char *getIp(struct addrinfo *address, uint16_t *port=nullptr);

    /// Perform DNS lookup of remote host.
    struct addrinfo *getAddressInfo(uint16_t iptype = 0);
protected:
    /// Return true unless last syscall encountered a fatal error.
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

    /// Return true if socket is watched for writeability.
    virtual bool wantToSend() {
        return false;
    }

    /// \brief
    /// This will be called regularly on READ_BLOCKED sockets to check if the
    /// block can be lifted.
    ///
    /// If your subclass ever returns READ_BLOCKED,
    /// it should override this method to return the new state when the block
    /// should be removed.
    virtual PollState checkReadBlock() {
        return PollState::READ_BLOCKED;
    }

    /// \brief
    /// Notify intention of sending large amounts of data.
    ///
    /// Normally, this is done simply by returning PollState::READ_WRITE
    /// from a scoket callback.
    /// This method is useful if you're not inside such a callback when you
    /// find out you need to send (large amounts of) data.
    void setWantToSend() {
        if (_state != PollState::CLOSE)
            _state = PollState::READ_WRITE;
    }

    /// Return file descriptor.
    int socket() const {
        return _socket;
    }

    /// Close a file descriptor.
    static int closeSocket(int fd);

    /// Tell the network engine that the connection should be closed.
    void closeMe() {
        _state = PollState::CLOSE;
    }

    /// \brief
    /// Create socket and initiate the connection.
    ///
    /// Will do no nothing if socket has already been created.
    /// On error, socket() will return -1.
    void createNonBlockingSocket(struct addrinfo *addressEntry,
                                 struct addrinfo *localAddr=nullptr);

    /// Set socket as non-blocking.
    bool setNonBlocking(int fd);

    /// Return true if the file descriptor has encountered a fatal error.
    static bool socketInError(int fd);

    /// Return true if the socket has encountered a fatal error.
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
