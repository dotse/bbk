// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <deque>
#include <map>

#include "../framework/bridgetask.h"

/// \brief
/// Client that only exists (or, rather, executes code) from within the bridge.
///
/// This way, it's trivial to create a non-interactive interface to the agent.
///
/// Shall be used with a SynchronousBridge.
class SynchronousClient {
public:
    /// \brief
    /// Send initial messages to the agent.
    ///
    /// Override this to push messages onto the queue.
    virtual void initialMsgToAgent(std::deque<std::string> &return_msgs);

    /// \brief
    /// Retrieve a new message from the agent.
    ///
    /// The client will only execute code from within its implementation
    /// of this method.
    ///
    /// The client must push any return messages onto return_msgs.
    virtual void newEventFromAgent(std::deque<std::string> &return_msgs,
                                   const std::string &msg) = 0;

    virtual ~SynchronousClient() { }
};

/// \brief
/// A bridge that "owns" the client.
///
/// The client only exists (or, rather, executes code) from within the bridge.
///
/// Shall be used with a SynchronousClient.
class SynchronousBridge : public BridgeTask {
public:
    SynchronousBridge(Task *agent, SynchronousClient *client) :
        BridgeTask(agent),
        the_client(client) {
    }

    /// See Task::start.
    double start() override;

    /// Will call the client's SynchronousClient::newEventFromAgent method.
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
