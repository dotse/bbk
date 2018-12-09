#pragma once

#include <framework/task.h>

class EchoServerTask : public Task {
public:
    EchoServerTask(uint16_t port);

    double start() override;

    double timerEvent() override;

    // Whenever a new client connects to our server socket, the eventloop
    // will call the below method. To reject the connection, we could return
    // nullptr. If we accept the connection we must create and return an object
    // belonging to a subclass of SocketConnection.
    SocketConnection *newClient(int fd, const char *ip,
                                uint16_t port, ServerSocket *) override;

    // The eventloop will call this method to notify us each time a new
    // SocketConnection has been added, i.e. after the newClient call.
    void connAdded(SocketConnection *conn) override;

    // The eventloop will call this method to notify us each time an existing
    // SocketConnection has been removed.
    void connRemoved(SocketConnection *conn) override;

    // The eventloop will call this method to notify us each time a new
    // server (listening) socket has been added.
    void serverAdded(ServerSocket *conn) override;

    // The eventloop will call this method to notify us each time an existing
    // server (listening) socket has been removed.
    void serverRemoved(ServerSocket *conn) override;

private:
    uint16_t port_number;
    unsigned int no_clients = 0;
};
