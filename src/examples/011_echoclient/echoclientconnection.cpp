#include "echoclientconnection.h"
#include "echoclienttask.h"

EchoClientConnection::
EchoClientConnection(EchoClientTask *task, const std::string &hostname,
                     unsigned int port) :
    SocketConnection("EchoClientConnection", task, hostname, port) {
}

PollState EchoClientConnection::connected() {
    msg_out = "Hi ho, hi ho";
    log() << "Socket connected, sending " << msg_out;
    asyncSendData(msg_out);
    return PollState::READ;
}

PollState EchoClientConnection::readData(char *buf, size_t len) {
    log() << "Got data: " << std::string(buf, len);
    msg_in.append(buf, len);
    if (msg_in.size() == msg_out.size()) {
        if (msg_in == msg_out) {
            log() << "Full response read. Will take a short nap.";
            if (EchoClientTask *t = dynamic_cast<EchoClientTask *>(owner()))
                t->test_succeeded();
        } else {
            log() << "Unexpected response. Nevermind. Will try again soon.";
        }
        return PollState::KEEPALIVE;
    } else if (msg_in.size() < msg_out.size()) {
        log() << "More data needed";
        return PollState::READ;
    } else {
        log() << "Got unexpected data: sent " << msg_out << ", got " << msg_in;
        return PollState::CLOSE;
    }
}
