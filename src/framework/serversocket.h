// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "socket.h"
#include "task.h"

class SocketConnection;

/// \brief
/// Listen on a single socket for incoming connections.
///
/// The owner task decides what to do when someone tries to connect.
class ServerSocket : public Socket {
public:
    /// \brief
    /// Create a new server socket.
    ///
    /// Will listen on the given ip address and port number.
    ///
    /// *Note!* If ip is an empty string, the socket will listen
    /// on all local IPv4 and IPv6 addresses.
    ServerSocket(const std::string &label, Task *task,
                 uint16_t port, const std::string &ip = "127.0.0.1");

    /// \brief
    /// Create a new server to listen on an existing file descriptor.
    ServerSocket(int fd, const std::string &label, Task *owner) :
        Socket(label, owner, fd) {
    }

    virtual ~ServerSocket() override;

    /// Server sockets shall not be cached.
    std::string cacheLabel() override {
        return std::string();
    }

    /// Schedule listen socket for removal.
    void stopListening() {
        closeMe();
    }

    /// Return Connection object if new client available, else return nullptr.
    virtual SocketConnection *incoming();

#ifdef USE_GNUTLS
    void tlsSetKey(unsigned int i) {
        tlsKeyIndex = i;
    }
    unsigned int tlsKey() const {
        return tlsKeyIndex;
    }
private:
    unsigned int tlsKeyIndex = 0;
#endif
};
