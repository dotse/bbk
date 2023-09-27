// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#ifdef _WIN32
#include <WS2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "serversocket.h"
class Task;

ServerSocket::ServerSocket(const std::string &label, Task *task,
                           uint16_t port, const std::string &ip) :
    Socket(label, task, ip, port) {
}

ServerSocket::~ServerSocket() {
}

SocketConnection *ServerSocket::incoming() {
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen = sizeof(remoteaddr);
    int fd = accept(socket(),
                    reinterpret_cast<sockaddr *>(&remoteaddr), &addrlen);
    if (fd < 0) {
        if (errno == EMFILE)
            Engine::notifyOutOfFds();
        errno_log() << "accept failure on socket " << socket();
        return nullptr;
    }
    if (!setNonBlocking(fd)) {
        log() << "cannot set non-blocking " << fd;
        closeSocket(fd);
        return nullptr;
    }
    uint16_t port;
    const char *ip = getIp(reinterpret_cast<sockaddr *>(&remoteaddr), &port);
    log() << "Incoming socket " << fd << " from " << ip << " port " << port;

    SocketConnection *conn = owner()->newClient(fd, ip, port, this);
    if (!conn)
        closeSocket(fd);
    return conn;
}
