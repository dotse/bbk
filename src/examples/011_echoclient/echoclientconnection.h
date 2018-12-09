#pragma once

#include <framework/socketconnection.h>

class EchoClientTask;

class EchoClientConnection : public SocketConnection {
public:
    EchoClientConnection(EchoClientTask *task, const std::string &hostname,
                         unsigned int port);
    PollState connected() override;

    PollState readData(char *buf, size_t len) override;
private:
    size_t sent = 0;
    std::string msg_in, msg_out;
};
