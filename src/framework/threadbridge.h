// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "bridgetask.h"
#include "msgqueue.h"

class ThreadBridge : public BridgeTask {
public:
    ThreadBridge(Task *agent = nullptr, double tick=0.05);

    // API for client. popFromAgent will return an empty string if no message
    // is available. Poll regularly.
    void pushToAgent(const std::string &msg);
    std::string popFromAgent();

    void sendMsgToClient(const std::string &msg) override;
    double start() override;
    double timerEvent() override;
private:
    MsgQueue<std::string> method_queue, event_queue;
    double tick_len;
};
