// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <sys/socket.h>
#include "serversocket.h"
class SocketConnection;

class SocketReceiver : public ServerSocket {
public:
    SocketReceiver(Task *task, int sock, pid_t peer_pid);

    // Return connection object if new client available, else return nullptr.
    virtual SocketConnection *incoming();

    bool passSocketToPeer(int fd);

    // Send data to peer. Return amount sent, or < 0 for error.
    ssize_t passMessageToPeer(const char *buf, size_t len);

    pid_t peerPid() const {
        return peer;
    }

    void peerDead() {
        peer = 0;
        closeMe();
    }
private:
    pid_t peer;

    // Used when passing file descriptors to peer:
    struct msghdr fdpass_msg;
    struct iovec empty_data;
    char cmsgbuf[CMSG_SPACE(sizeof(int))];
    struct cmsghdr *cmsg;

    // Used when sending data to peer:
    struct msghdr parent_msg;
    struct iovec msg_data;
};
