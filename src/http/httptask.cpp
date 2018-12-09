// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#include "httptask.h"
#include "httpconnection.h"

// New websocket text message
bool HttpTask::wsTextMessage(HttpConnection *,
                             const std::string &msg) {
    log() << "ignoring websocket text message: " << msg;
    return false;
}
bool HttpTask::wsBinMessage(HttpConnection *,
                            const std::string &msg) {
    log() << "ignoring websocket bin message, length " << msg.size();
    return false;
}

size_t HttpTask::sendWsData(HttpConnection *conn) {
    conn->abortWsStream();
    return 0;
}
