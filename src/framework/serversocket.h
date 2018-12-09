// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

// Class to listen on a single socket for incoming connections.
// The owner task decides what to do when someone tries to connect.

#pragma once

#include "socket.h"
#include "task.h"

class SocketConnection;

class ServerSocket : public Socket {
public:
    ServerSocket(const std::string &label, Task *task,
                 uint16_t port, const std::string &ip = "127.0.0.1");

    ServerSocket(const std::string &label, Task *owner, int fd) :
        Socket(label, owner, fd) {
    }

    virtual ~ServerSocket() override;

    std::string cacheLabel() override {
        return std::string();
    }

    void stopListening() {
        closeMe();
    }

    // Return Connection object if new client available, else return nullptr.
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
