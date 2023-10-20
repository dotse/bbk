// Copyright (c) 2019 Internetstiftelsen
// Written by Göran Andersson <initgoran@gmail.com>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include "unixdomainclient.h"
#include "bridgetask.h"

void UnixDomainClient::pushToAgent(const std::string &msg) {
    to_agent += std::to_string(msg.size());
    to_agent += '\n';
    to_agent += msg;
    auto n = send(client_socket, to_agent.c_str(), to_agent.size(), 0);
    if (n > 0)
        to_agent.erase(0, n);
}

std::string UnixDomainClient::pollAgent() {
    ssize_t n;
    while (true) {
        char buffer[50000];
        n = recv(client_socket, buffer, sizeof buffer, 0);
        if (n <= 0)
            break;
        to_client.append(buffer, n);
        // Maybe we should have a max size for a single message.
    }
    auto err = errno;

    auto pos = to_client.find('\n');
    if (pos != std::string::npos) {
        // Check if a new (complete) message has arrived.
        try {
            auto msg_len = std::stoul(to_client.substr(0, pos));
            if (to_client.size() > pos + msg_len) {
                ++pos;
                std::string result = to_client.substr(pos, msg_len);
                to_client.erase(0, pos+msg_len);
                return result;
            }
        } catch (...) {
            return BridgeTask::agentTerminatedMessage("bad data");
        }
    }

    if (n == 0)
        return BridgeTask::agentTerminatedMessage("lost connection");
    if (err && err != EAGAIN && err != EWOULDBLOCK &&
        err != EINPROGRESS && err != EINTR) {
        return BridgeTask::agentTerminatedMessage("connection error");
    }
    return "";
}

bool UnixDomainClient::flushToAgent() {
    if (to_agent.empty())
        return true;
    auto n = send(client_socket, to_agent.c_str(), to_agent.size(), 0);
    if (n > 0) {
        to_agent.erase(0, n);
        return to_agent.empty();
    }
    return false;
}

std::string UnixDomainClient::waitForMsgFromAgent(unsigned long timeout_us) {

    std::string msg = pollAgent();
    if (!msg.empty())
        return msg;

    struct timeval timeout;
    timeout.tv_sec = timeout_us/1000000;
    timeout.tv_usec = timeout_us%1000000;

    fd_set readFds, errFds;
    FD_ZERO(&readFds);
    FD_ZERO(&errFds);
    FD_SET(client_socket, &readFds);
    FD_SET(client_socket, &errFds);
    select(client_socket + 1, &readFds, nullptr, &errFds,
           timeout_us ? &timeout : nullptr);

    return pollAgent();
}
