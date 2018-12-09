// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#ifndef _WIN32

#include "socketreceiver.h"

SocketReceiver::SocketReceiver(Task *task, int sock, pid_t peer_pid) :
    ServerSocket("SocketReceiver", task, sock),
    peer(peer_pid) {

    empty_data.iov_base = nullptr;
    empty_data.iov_len = 0;

    memset(&fdpass_msg, 0, sizeof(fdpass_msg));
    fdpass_msg.msg_iov = &empty_data;
    fdpass_msg.msg_iovlen = 1;

    fdpass_msg.msg_control = cmsgbuf;
    fdpass_msg.msg_controllen = sizeof(cmsgbuf);
    cmsg = CMSG_FIRSTHDR(&fdpass_msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    fdpass_msg.msg_controllen = cmsg->cmsg_len;

    memset(&parent_msg, 0, sizeof(parent_msg));
    parent_msg.msg_iov = &msg_data;
    parent_msg.msg_iovlen = 1;
}

SocketConnection *SocketReceiver::incoming() {
    //log() << "SocketReceiver::new_client_socket()";
    struct msghdr child_msg;

    memset(&child_msg, 0, sizeof(child_msg));
    child_msg.msg_control = cmsgbuf;
    child_msg.msg_controllen = sizeof(cmsgbuf);

    static thread_local char buf[10000];
    static thread_local struct iovec data = { buf, sizeof(buf) };

    child_msg.msg_iov = &data;
    child_msg.msg_iovlen = 1;

    ssize_t len = recvmsg(socket(), &child_msg, MSG_DONTWAIT);
    if (len < 0) {
        log() << "recvmsg() failed: " << strerror(errno);
        return nullptr;
    }
    cmsg = CMSG_FIRSTHDR(&child_msg);
    if (cmsg == nullptr || cmsg->cmsg_type != SCM_RIGHTS) {
        //log() << "Error: not a file descriptor";
        if (child_msg.msg_iovlen && owner()) {
            struct iovec data1 = child_msg.msg_iov[0];
            auto addr = reinterpret_cast<const char *>(data1.iov_base);
            owner()->workerMessage(this, addr, static_cast<size_t>(len));
        }
        return nullptr;
    }

    int newfd;
    memcpy(&newfd, CMSG_DATA(cmsg), sizeof(newfd));

    uint16_t port;
    const char *ip = Socket::getIp(newfd, &port);
    dbg_log() << "Received socket " << newfd << " from " << ip
              << " port " << port;

    return owner()->newClient(newfd, ip, port, this);
}

bool SocketReceiver::passSocketToPeer(int fd) {
    memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));

    log() << "passing fd " << fd << " to peer";
    ssize_t ret = sendmsg(socket(), &fdpass_msg, MSG_DONTWAIT);
    closeSocket(fd);
    if (ret < 0) {
        if (errno == EAGAIN) {
            log() << "cannot pass socket: job queue full";
            // The peer is probably broken. Kill it.
            closeMe();
        } else {
            errno_log() << "cannot pass socket";
        }
        return false;
    }
    return true;
}

ssize_t SocketReceiver::passMessageToPeer(const char *buf, size_t len) {
    msg_data.iov_base = const_cast<char *>(buf);
    msg_data.iov_len = len;
    ssize_t sent = sendmsg(socket(), &parent_msg, 0);
    if (sent != static_cast<ssize_t>(len))
        log() << "only sent " << sent << " of total " << len;
    return sent;
}

#endif
