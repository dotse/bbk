// Copyright (c) 2019 Internetstiftelsen
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <string>

class UnixDomainClient {
public:
    UnixDomainClient(int peer_fd) :
        client_socket(peer_fd) {
    }
    // popFromAgent will return an empty string if no message
    // is available. Poll regularly or monitor the socket descriptor.
    std::string pollAgent();

    // As above, but blocks until a message is available
    std::string waitForMsgFromAgent();

    void pushToAgent(const std::string &msg);

private:
    std::string to_agent;
    std::string to_client;
    int client_socket;
};
