// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <sys/socket.h>
#include "serversocket.h"
class SocketConnection;

/// \brief
/// Pass sockets and messages between processes.
///
/// Typical use is to have a master process listen on a port,
/// passing any new connections to child processes.
///
/// Both sockets and messages (data) may be passsed between processes.
///
/// A pair of Unix domain sockets must be created in the parent process,
/// to be used by one SocketReceiver in the parent and one SocketReceiver
/// in the child process.
class SocketReceiver : public ServerSocket {
public:
    /// Add a SocketReceiver to the given Task.
    /// The `sock` parameter shall be one of a pair of Unix domain sockets.
    /// The `peer_pid` parameter is used only for logging.
    SocketReceiver(Task *task, int sock, pid_t peer_pid);

    /// Return connection object if new client available, else return nullptr.
    virtual SocketConnection *incoming() override;

    /// Return 0 for success, errno on failure
    int passSocketToPeer(int fd);

    /// Send data to peer. Return amount sent, or < 0 for error.
    ssize_t passMessageToPeer(const char *buf, size_t len);

    /// Return PID of the peer process.
    pid_t peerPid() const {
        return peer;
    }

    /// Stop communication with peer process and terminate as soon as possible.
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
