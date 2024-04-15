// Copyright (c) 2019 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#include "task.h"
#include "shortmessageconnection.h"

ShortMessageConnection::
ShortMessageConnection(const std::string &label, Task *owner,
                       const std::string &hostname, uint16_t port) :
    SocketConnection(label, owner, hostname, port) {
}


ShortMessageConnection::
ShortMessageConnection(const std::string &label, Task *owner, int fd,
                       const char *ip, uint16_t port) :
    SocketConnection(label, owner, fd, ip, port) {
}

PollState ShortMessageConnection::connected() {
    return owner()->connectionReady(this);
}

PollState ShortMessageConnection::readData(char *buf, size_t len) {

    if (reading_header) {
        while (len && *buf >= '0' && *buf <= '9') {
            (bytes_left *= 10) += (*buf++ - '0');
            --len;
        }
        if (!len)
            return PollState::READ;
        if (!bytes_left || *buf != '\n') {
            err_log() << "Got unexpected data";
            return PollState::CLOSE;
        }
        reading_header = false;
        ++buf;
        --len;
    }

    if (len < bytes_left) {
        msg.append(std::string(buf, len));
        bytes_left -= len;
        return PollState::READ;
    }

    msg.append(std::string(buf, bytes_left));
    len -= bytes_left;
    buf += bytes_left;
    bytes_left = 0;

    if (tellOwner(msg) == PollState::CLOSE)
        return PollState::CLOSE;

    msg.clear();
    reading_header = true;

    if (len)
        return readData(buf, len);

    return PollState::READ;
}

void ShortMessageConnection::sendMessage(const std::string &msg) {
    std::string s = std::to_string(msg.size()) + '\n';
    asyncSendData(s + msg);
}
