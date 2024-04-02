// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#include "threadbridge.h"
#include <iostream>

ThreadBridge::ThreadBridge(Task *agent, double tick) :
    BridgeTask(agent),
    tick_len(tick) {
}

double ThreadBridge::start() {
    BridgeTask::start();
    return tick_len;
}

double ThreadBridge::timerEvent() {
    std::string msg;
    while (true) {
        method_queue.fetch(msg);
        if (msg.empty())
            return tick_len;
        sendMsgToAgent(msg);
        msg.clear();
    }
}

void ThreadBridge::pushToAgent(const std::string &msg) {
    dbg_log() << "To agent: " << msg;
    method_queue.push(msg);
}

std::string ThreadBridge::popFromAgent() {
    std::string msg;
    event_queue.fetch(msg);
    return msg;
}

void ThreadBridge::sendMsgToClient(const std::string &msg) {
    dbg_log() << "To client: " << msg;
    event_queue.push(msg);
}
