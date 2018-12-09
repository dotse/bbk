// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

// If a task needs to communicate with an application running outside the
// event loop, this class provides an abstract interface for the communication.

// The other application might run in another process or thread. The actual
// means of communication might be sockets or pipes, or shared variables in the
// case of threads, or something else entirely.

// The task running in the event loop is called the "agent", and the outside
// aplication is called the "client".

// To use this class, you must
//    - derive from this class
//    - implement the sendMessageToClient method
//    - override the taskFinished method to detect if the agent dies
//    - create an agent object and a bridge object
//    - add the bridge object to the event loop
// The agent task will be aborted when the bridge task is finished.
// If you need timers (e.g. for polling), you must override the start() method
// below and your start() method must call BridgeTask::start().

// When there is a message from the client to the agent, the agent's
// handleExecution method will be called with two parameters: the bridge object
// and a message.

// Typically, the agent's handleExecution method checks if the first parameter
// can be cast to a BridgeTask pointer. If so, the second parameter is a message
// from the client. When the first message from the client arrives, the agent
// stores the pointer to the bridge in case it needs to send messages back.


#pragma once

#include <string>
#include "task.h"

class BridgeTask : public Task {

public:
    BridgeTask(Task *agent = nullptr) : Task("Bridge"), the_agent(agent) {
        killChildTaskWhenFinished();
    }

    double start() override;

    // If the agent dies, we send a special message to notify the client.
    void taskFinished(Task *task) override {
        if (task == the_agent) {
            if (!task->result().empty()) {
                log() << "Agent terminated";
                sendMsgToClient("AGENT EXIT: " + task->result());
            }
            die();
        }
    }

    void setAgent(Task *agent) {
        if (!the_agent && !hasStarted())
            the_agent = agent;
        else
            err_log() << "cannot set agent";
    }

    void die() {
        setResult("");
    }

    // The agent will call this to pass messages to the client:
    virtual void sendMsgToClient(const std::string &msg) = 0;

    // Recommended way to format messages to the agent: a method name,
    // and "arguments" stored in a JSON object.
    static std::string msgToAgent(const std::string &method,
                                  const std::string &jsonobj = "{}") {
        return "{\"method\": \"" + method + "\", \"args\": " + jsonobj + "}";
    }

    // Recommended way to format messages to the client: an event (or method)
    // name, and "arguments" stored in a JSON object.
    void sendMsgToClient(const std::string &method, const std::string &jsonobj) {
        sendMsgToClient("{\"event\": \"" + method +
                        "\", \"args\": " + jsonobj + "}");
    }

    virtual ~BridgeTask() override;

protected:
    void sendMsgToAgent(const std::string &msg) {
        executeHandler(the_agent, msg);
        if (msg == terminate_msg)
            die();
    }

    void sendMsgToAgent(const std::string &method, const std::string &jsonobj) {
        sendMsgToAgent(msgToAgent(method, jsonobj));
    }
private:
    Task *the_agent;
    //const std::string quit_msg = "quit";
    const std::string terminate_msg = "terminate";
};
