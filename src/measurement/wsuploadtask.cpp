// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#include "wsuploadtask.h"
#include <vector>

WsUploadTask::WsUploadTask(const std::string &ticket, const HttpHost &server,
                       unsigned int no_conn, unsigned int max_conn,
                       double duration, double max_time, double tick_s) :
    ProgressTask("upload", ticket, server, no_conn, max_conn, duration, max_time),
    tick_duration_s(tick_s>0 ? tick_s : 0.1) {
}

double WsUploadTask::start() {
    log() << "WsUploadTask starting";
    checkConnectionCount();
    return tick_duration_s;
}

void WsUploadTask::newRequest(HttpClientConnection *conn) {
    // conn->dbgOn();
    conn->ws_get("/ws?t=" + t());
}

namespace {
    std::vector<char> getUploadBuffer(size_t len) {
        std::vector<char> buf;
        // Fill the buffer with dummy data that cannot cause data compression
        while (len) {
            buf.push_back(static_cast<char>(rand()));
            --len;
        }
        return buf;
    }
}

bool WsUploadTask::nextRequest(HttpConnection *conn) {
    size_t len = loadSize();
    if (!len)
        return false;
    conn->startWsBinStream(len);
    return true;
}

size_t WsUploadTask::sendWsData(HttpConnection *conn) {
    static std::vector<char> buffer = getUploadBuffer(post_buffer_len);

    size_t to_write = std::min(conn->wsOutgoingBytesLeft(), post_buffer_len);
    //log() << "sendWsData " << to_write;
    return conn->sendData(buffer.data(), to_write);
}

double WsUploadTask::timerEvent() {
    double time = elapsed();

    if (time >= timeout_s()) {
        log() << "UploadTask timeout after " << time << " seconds";
        setResult("-1");
        return 0;
    }

    if (time > 0.05) {
        double mbps = addOverheadMbps(byteCount(), time);
        doTestProgress(mbps, time, currentNoConnections());
    }

    return tick_duration_s;
}

bool WsUploadTask::websocketUpgrade(HttpClientConnection *conn) {
    return nextRequest(conn);
}

bool WsUploadTask::wsBinMessage(HttpConnection *,
                                const std::string &msg) {
    log() << "Unexpected ws bin message, size=" << msg.size();
    return false;
}

bool WsUploadTask::wsTextMessage(HttpConnection *conn,
                                 const std::string &msg) {
    //log() << "WsUploadTask got ws text message: " << msg;
    std::istringstream ss(msg);
    std::string op;
    size_t count;
    if (ss >> op >> count && op == "upload") {
        notifyBytesLoaded(count);
        return nextRequest(conn);
    }
    return false;
}

const size_t WsUploadTask::post_buffer_len;
