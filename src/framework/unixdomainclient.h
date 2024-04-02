// Copyright (c) 2019 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <string>

/// \brief
/// Client communicating with an agent task using a UnixDomainBridge.
///
/// This class is meant to be used only _outside_ the event loop, i.e. from some
/// other thread or process. It is used together with a UnixDomainBridge, which
/// runs in the event loop. After creating the UnixDomainBridge object, call
/// getClientSocket() on it to get a file descriptor which should be used as
/// an argument to the constructor of this class. Then make sure the
/// UnixDomainBridge object runs in an event loop, but not in the same thread or
/// process as this class.
class UnixDomainClient {
public:
    /// Create client using one of a pair of Unix domain sockets.
    UnixDomainClient(int peer_fd) :
        client_socket(peer_fd) {
    }

    /// \brief
    /// Return a line of data from the agent.
    ///
    /// Will return an empty string if no message is available.
    ///
    /// Poll regularly or monitor the socket descriptor.
    std::string pollAgent();

    /// As UnixDomainClient::pollAgent, but block until a message is available
    /// or until timeout_us
    /// microseconds have passed (or forever, if timeout_us is 0).
    std::string waitForMsgFromAgent(unsigned long timeout_us = 0);

    /// \brief
    /// Send message to agent.
    ///
    /// *Note:* If the message is large (perhaps >200KB),
    /// it's likely that the complete message can't be delivered to the agent
    /// immediately. In that case, it will be buffered and you may have to
    /// call pushToAgent (with new messages) or flushToAgent() repeatedly until
    /// the buffer is drained. (Of course, unless the agent actively reads the
    /// messages, the buffer can't be drained.)
    void pushToAgent(const std::string &msg);

    /// \brief If there is unsent data, retry sending it to the agent.
    ///
    /// Return true if agent has received all messages we sent.
    ///
    /// If the return value is false,
    /// you must call flushToAgent again at a later time (e.g.
    /// after 50ms, or preferably when the socket is found to be writable.)
    bool flushToAgent();

private:
    std::string to_agent;
    std::string to_client;
    int client_socket;
};
