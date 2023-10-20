// Copyright (c) 2019 Internetstiftelsen
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "socketconnection.h"

/// \brief
/// Simple protocol for exchanging messages.
///
/// Use sendMessage to send a message to peer.
///
/// When a message has been received from peer, it will be passed to
/// the owner task's msgFromConnection method.
/// When the connection is ready (connected), the owner task's connectionReady
/// method will be called.
/// The connectionReady and msgFromConnection methods must return
/// PollState::READ to keep the connection, or PollState::CLOSE to close it.
class ShortMessageConnection : public SocketConnection {
public:
    /// For client sockets, connecting to a server.
    ShortMessageConnection(const std::string &label, Task *owner,
                         const std::string &hostname, uint16_t port);

    /// For already connected sockets, i.e. in a server.
    ShortMessageConnection(const std::string &label, Task *owner, int fd,
                           const char *ip = "unknown", uint16_t port = 0);

    /// Will notify owner task that a new connection is available.
    PollState connected() override;

    /// Will notify owner when a complete message has been retrieved.
    PollState readData(char *buf, size_t len) override;

    /// Send message to peer.
    void sendMessage(const std::string &msg);

private:
    // Message we're currently receiving, or empty.
    std::string msg;

    // Bytes left for the above message to be complete,
    // or 0 if we're not currently receiving a message
    size_t bytes_left = 0;

    bool reading_header = true;
};
