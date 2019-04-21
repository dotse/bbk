// Copyright (c) 2019 Internetstiftelsen
// Written by Göran Andersson <initgoran@gmail.com>

#include <sys/types.h>
#include <sys/socket.h>

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
    char buffer[4000];
    auto n = recv(client_socket, buffer, sizeof buffer, 0);
    auto err = errno;
    if (n < 0) {
        if (err && err != EAGAIN && err != EWOULDBLOCK &&
            err != EINPROGRESS && err != EINTR) {
            std::cerr << "\nerror: " << strerror(errno) << std::endl;
            return BridgeTask::agentTerminatedMessage("connection error");
        }
    } else if (n == 0) {
        return BridgeTask::agentTerminatedMessage("lost connection");
    } else {
        to_client.append(buffer, n);
    }
    auto pos = to_client.find('\n');
    if (pos == std::string::npos)
        return "";
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
    return pollAgent();
}
