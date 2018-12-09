#include "echoservertask.h"
#include "echoserverconnection.h"
#include <framework/serversocket.h>

EchoServerTask::EchoServerTask(uint16_t port) :
    Task("Echo Server"),
    port_number(port) {
}

double EchoServerTask::start() {
    // The ServerSocket object is owned by the eventloop and will be deleted
    // by the eventloop when it's no longer needed.
    if (addServer(new ServerSocket("Echo Listen Socket", this, port_number)))
        return 2.0;
    setResult("Failed, cannot start server");
    return 0;
}

double EchoServerTask::timerEvent() {
    log() << "Current number of clients: " << no_clients;
    return 5.0;
}

SocketConnection *EchoServerTask::newClient(int fd, const char *ip,
                                            uint16_t port, ServerSocket *) {
    // The EchoServerConnection object is owned by the eventloop and will be
    // deleted by the eventloop when it's no longer needed.
    // Just before deleting it, the eventloop will notify us by calling the
    // below connRemoved method.
    // We really shoudn't keep local copies of the (pointers to the) connection
    // objects, but if we still do so, we _must_ implement connRemoved to delete
    // our local copies so we don't use them after the objects are deleted.
    return new EchoServerConnection(this, fd, ip, port);
}

void EchoServerTask::connAdded(SocketConnection *) {
    log() << "added client";
    ++no_clients;
}

void EchoServerTask::connRemoved(SocketConnection *) {
    log() << "lost client";
    --no_clients;
}

void EchoServerTask::serverAdded(ServerSocket *) {
    log() << "listen socket added";
}

void EchoServerTask::serverRemoved(ServerSocket *) {
    setResult("Server hangup!");
}
