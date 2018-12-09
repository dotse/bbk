// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <deque>
#include <map>

#include "../framework/bridgetask.h"

class SynchronousClient {
public:
    // initial messages to the agent shall be pushed onto return_msgs.
    virtual void initialMsgToAgent(std::deque<std::string> &return_msgs);

    // msg is a new message from the agent.
    // push any return messages onto return_msgs.
    virtual void newEventFromAgent(std::deque<std::string> &return_msgs,
                                   const std::string &msg) = 0;

    // Returns true if msg is formatted as a note that the agent has terminated
    // and that there will be no more messages.
    static bool isTerminateMessage(const std::string &msg) {
        return (msg.substr(0, 12) == "AGENT EXIT: ");
    }

    virtual ~SynchronousClient() { }
};

class SynchronousBridge : public BridgeTask {
public:
    SynchronousBridge(Task *agent, SynchronousClient *client) :
        BridgeTask(agent),
        the_client(client) {
    }

    double start() override;

    void sendMsgToClient(const std::string &msg) override;

    virtual ~SynchronousBridge() override;
private:
    void clear_queue() {
        while (!incoming_messages.empty()) {
            std::string msg = incoming_messages.front();
            incoming_messages.pop_front();
            log() << "sendMsgToAgent " << msg;
            sendMsgToAgent(msg);
        }
    }
    SynchronousClient *the_client;
    std::deque<std::string> incoming_messages;
};
