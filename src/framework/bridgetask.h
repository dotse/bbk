// Copyright (c) 2019 Internetstiftelsen
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <string>
#include "task.h"

/// \brief
/// Tasks may use a bridge to communicate with an application running
/// outside the event loop.
///
/// This class provides an abstract interface for a _bridge_ which facilitates
/// communication between a task and code running outside the event loop.
/// The code might run in another process or thread. The actual
/// means of communication might be sockets or pipes, or shared variables in
/// the case of threads, or something else entirely.
///
/// The task running in the event loop is called the _agent_, and the outside
/// aplication is called the _client_.
///
/// To use this class, you must
///    - derive from this class
///    - implement the sendMessageToClient method
///    - implement a way for the client to send messages back, and pass those
///      messages through the sendMsgToAgent method
///    - if you override the start() method, call BridgeTask::start() first
///    - create an agent object and a bridge object
///    - add the bridge object to the event loop
/// The agent task will be aborted when the bridge task is finished.
/// If you need timers (e.g. for polling), you must override the start() method
/// below and your start() method must call BridgeTask::start().
///
/// When there is a message from the client to the agent, the agent's
/// Task::handleExecution method will be called with two parameters:
/// the bridge object and a message.
///
/// Typically, the agent's Task::handleExecution method checks if the
/// first parameter can be cast to a BridgeTask pointer.
/// If so, the second parameter is a message from the client.
/// When the first message from the client arrives, the agent stores
/// the pointer to the bridge in case it needs to send messages back.
class BridgeTask : public Task {

public:
    /// \brief
    /// Create a bridge to the given agent task.
    ///
    /// If the agent task isn't available yet, it must be added using
    /// BridgeTask::setAgent before the bridge is added to the EventLoop.
    BridgeTask(Task *agent = nullptr) : Task("Bridge"), the_agent(agent) {
        killChildTaskWhenFinished();
    }

    /// \brief
    /// Will add the agent task to the EventLoop.
    ///
    /// *Note!* If you override this method, it should be called explicitly by
    /// the overriding method!
    double start() override;

    /// If the agent dies, a special message will be sent to notify the client.
    void taskFinished(Task *task) override {
        if (task == the_agent) {
            if (!task->result().empty()) {
                log() << "Agent terminated";
                sendMsgToClient(agentTerminatedMessage(task->result()));
                the_agent = nullptr;
            }
            die();
        }
    }

    /// If the agent task was created after the bridge, it must be added using
    /// this method _before_ the bridge is added to the EventLoop.
    void setAgent(Task *agent) {
        if (!the_agent && !hasStarted())
            the_agent = agent;
        else
            err_log() << "cannot set agent";
    }

    /// Terminate the bridge task.
    void die() {
        setResult("");
    }

    /// The agent will call this to pass messages to the client.
    virtual void sendMsgToClient(const std::string &msg) = 0;

    /// \brief Format a message to the agent.
    ///
    /// Recommended way to format messages to the agent: a method name,
    /// and "arguments" stored in a JSON object.
    ///
    /// *Note!* The method name must not contain any special chararcters.
    static std::string msgToAgent(const std::string &method,
                                  const std::string &jsonobj = "{}") {
        return "{\"method\": \"" + method + "\", \"args\": " + jsonobj + "}";
    }

    /// Return true if msg is formatted as a note that the agent has terminated
    /// and that there will be no more messages.
    static bool isAgentTerminatedMessage(const std::string &msg) {
        return (msg.substr(0, 12) == "AGENT EXIT: ");
    }

    /// Format a message to signal that the Agent is gone.
    static std::string agentTerminatedMessage(const std::string &err_msg) {
        return "AGENT EXIT: " + err_msg;
    }

    /// \brief Format a message to the client.
    ///
    /// Recommended way to format messages to the client: an event (or method)
    /// name, and "arguments" stored in a JSON object.
    ///
    /// *Note!* The method name must not contain any special chararcters.
    void sendMsgToClient(const std::string &method, const std::string &jsonobj) {
        sendMsgToClient("{\"event\": \"" + method +
                        "\", \"args\": " + jsonobj + "}");
    }

    virtual ~BridgeTask() override;

protected:
    /// Send message to the agent.
    void sendMsgToAgent(const std::string &msg) {
        executeHandler(the_agent, msg);
        if (msg.substr(0, terminate_msg.size()) == terminate_msg)
            die();
    }

    /// Format and send message to the agent.
    void sendMsgToAgent(const std::string &method, const std::string &jsonobj) {
        sendMsgToAgent(msgToAgent(method, jsonobj));
    }
private:
    Task *the_agent;
    //const std::string quit_msg = "quit";
    const std::string terminate_msg = "{\"method\": \"terminate\"";
};
