// Copyright (c) 2019 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "bridgetask.h"
class ShortMessageConnection;

/// \brief
/// Bridge using a pair of Unix domain sockets to enable communication
/// between agent and client.
class UnixDomainBridge : public BridgeTask {
public:

    /// \brief
    /// Create a bridge to the given agent task.
    ///
    /// A pair of Unix domain sockets will be used to enable communication
    /// between agent and client.
    UnixDomainBridge(Task *agent = nullptr);

    /// Pass message to the client.
    void sendMsgToClient(const std::string &msg) override;

    /// \brief
    /// Get client's socket descriptor.
    ///
    /// Return 0 on failure.
    ///
    /// *Note:* client may run in another thread or process.
    /// Use in child, close in parent after fork.
    int getClientSocket() const;

    /// Close in child after fork.
    int getAgentSocket() const;

    /// See Task::connectionReady.
    PollState connectionReady(SocketConnection * /* conn */) override;

    /// Will be called when client has sent a message.
    PollState msgFromConnection(SocketConnection * /* conn */,
                                const std::string &msg) override;

    /// See Task::start.
    double start() override;

private:
    ShortMessageConnection *msg_conn;
};
