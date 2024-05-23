// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "bridgetask.h"
#include "msgqueue.h"

/// \brief
/// Bridge using a thread safe queue to enable communication
/// between agent and client.
class ThreadBridge : public BridgeTask {
public:
    ThreadBridge(Task *agent = nullptr, double tick=0.05);

    /// API for client to send message to the agent.
    void pushToAgent(const std::string &msg);

    /// \brief
    /// API for client to retrieve next message from agent.
    ///
    /// If no messages are available, an empty string will be returned.
    /// Client shoud call this regularly.
    std::string popFromAgent();

    /// Pass message to the client.
    void sendMsgToClient(const std::string &msg) override;

    /// Initiate timer to be called after `tick` seconds.
    double start() override;

    /// Push queued messages for the agent. Called every `tick` seconds.
    double timerEvent() override;
private:
    MsgQueue<std::string> method_queue, event_queue;
    double tick_len;
};
