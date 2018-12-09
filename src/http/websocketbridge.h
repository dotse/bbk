// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <deque>
#include <atomic>

#include "../framework/bridgetask.h"
#include "../http/webservertask.h"
#include "../framework/serversocket.h"

class WSBlistener;

class WebsocketBridge : public BridgeTask {
public:
    WebsocketBridge(Task *agent, const TaskConfig &cfg);

    virtual ~WebsocketBridge() override;

    double start() override;

    void sendMsgToClient(const std::string &msg) override;

    void handleExecution(Task *sender, const std::string &msg) override;

    void taskFinished(Task *task) override;

    // Returns empty string unless ready. Thread safe.
    std::string url() const;

private:
    WSBlistener *listen_task = nullptr;
    std::atomic<uint16_t> listen_port;
};
