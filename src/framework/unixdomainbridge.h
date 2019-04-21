// Copyright (c) 2019 Internetstiftelsen
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "bridgetask.h"
class ShortMessageConnection;

class UnixDomainBridge : public BridgeTask {
public:

    UnixDomainBridge(Task *agent = nullptr);

    void sendMsgToClient(const std::string &msg) override;

    // Get client's socket descriptor. Return 0 on failure.
    // Note: client may run in another thread or process.
    // Use in child, close in parent after fork:
    int getClientSocket() const;

    // Close in child after fork:
    int getAgentSocket() const;

    PollState connectionReady(SocketConnection * /* conn */) override;

    // Will be called when client has sent a message.
    PollState msgFromConnection(SocketConnection * /* conn */,
                                const std::string &msg) override;

    double start() override;

private:
    ShortMessageConnection *msg_conn;
};
