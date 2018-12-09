#pragma once

#include <framework/socketconnection.h>

class EchoServerConnection : public SocketConnection {
public:
    EchoServerConnection(Task *task, int fd, const char *ip, uint16_t port);

    // Called by the eventloop when a client connection has been established.
    PollState connected() override;

    // Called by the eventloop when data has arrived from the client.
    PollState readData(char *buf, size_t len) override;

    // Called regularly by the eventloop if we have blocked incoming data,
    // i.e. if we have returned PollState::READ_BLOCKED from the above method.
    PollState checkReadBlock() override;
private:
};
