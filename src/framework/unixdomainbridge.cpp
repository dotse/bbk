// Copyright (c) 2019 Internetstiftelsen
// Written by Göran Andersson <initgoran@gmail.com>

#include "unixdomainbridge.h"
#include "shortmessageconnection.h"

UnixDomainBridge::UnixDomainBridge(Task *agent) :
    BridgeTask(agent),
    msg_conn(new ShortMessageConnection("UDBridgeConn", this, "UnixDomain", 0)) {
}

double UnixDomainBridge::start() {
    BridgeTask::start();
    addConnected(msg_conn);
    return 0;
}

void UnixDomainBridge::sendMsgToClient(const std::string &msg) {
    msg_conn->sendMessage(msg);
}

int UnixDomainBridge::getClientSocket() const {
    if (!msg_conn)
        return 0;
    return msg_conn->getUnixDomainPeer();
}

int UnixDomainBridge::getAgentSocket() const {
    if (!msg_conn || msg_conn->id() < 0)
        return 0;
    return msg_conn->id();
}

PollState UnixDomainBridge::connectionReady(SocketConnection * /* conn */) {
    return PollState::READ;
}

PollState UnixDomainBridge::msgFromConnection(SocketConnection *,
                                              const std::string &msg) {
    sendMsgToAgent(msg);
    return PollState::READ;
}
