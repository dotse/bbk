// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#include <queue>

#include "synchronousbridge.h"

void SynchronousClient::initialMsgToAgent(std::deque<std::string> &) {
    // Override this to push messages onto the queue.
}

double SynchronousBridge::start() {
    dbg_log() << "starting";
    BridgeTask::start();
    the_client->initialMsgToAgent(incoming_messages);
    clear_queue();
    return 0;
}

SynchronousBridge::~SynchronousBridge() {
}

void SynchronousBridge::sendMsgToClient(const std::string &msg) {
    // The client executes code only from within the below call.
    the_client->newEventFromAgent(incoming_messages, msg);
    clear_queue();
}
