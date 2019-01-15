// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#include "uploadtask.h"
#include <vector>

UploadTask::UploadTask(const std::string &ticket, const HttpHost &server,
                       unsigned int no_conn, unsigned int max_conn,
                       double duration, double max_time, double tick_s) :
    ProgressTask("upload", ticket, server, no_conn, max_conn, duration, max_time),
    tick_duration_s(tick_s>0 ? tick_s : 0.1) {
}

double UploadTask::start() {
    log() << "UploadTask starting";
    notifyStarted();
    checkConnectionCount();
    return tick_duration_s;
}

void UploadTask::newRequest(HttpClientConnection *conn) {
    if (size_t to_write = loadSize()) {
        conn->post("/cgi/upload.cgi?t=" + t() + "&id=1", to_write);
        post_size[conn] = to_write;
    } else {
        conn->pass();
    }
}

bool UploadTask::requestComplete(HttpClientConnection *conn) {
    if (conn->httpStatus() == 200) {
        // Now we know for sure that all posted data has arrived at the server
        notifyBytesLoaded(post_size[conn]);
    }
    return true;
}

double UploadTask::timerEvent() {
    double time = elapsed();

    if (time >= timeout_s()) {
        log() << "UploadTask timeout after " << time << " seconds";
        setResult("-1");
        return 0;
    }

    if (time > 0.05) {
        // log() << "Check: " << byteCount() << " and " << threadSendCount();

        // Note: byteCount() is the number of bytes we know for sure have
        // arrived at the server, threadSendCount() is the number of bytes
        // we've put into socket buffers. We have to be careful and use the
        // lower value, i.e. byteCount(), which is too low, instead of the
        // higher value, i.e. threadSendCount(), which is too large.
        // However, the server knows the correct value and will send it to
        // us regularly through the InfoTask.

        double mbps = addOverheadMbps(byteCount(), time);
        doTestProgress(mbps, time, currentNoConnections());
    }

    return tick_duration_s;
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

size_t UploadTask::doPost(HttpClientConnection *conn, size_t len) {
    static std::vector<char> buffer = getUploadBuffer(post_buffer_len);
    size_t to_write = std::min(len, post_buffer_len);
    return conn->sendData(buffer.data(), to_write);
}

const size_t UploadTask::post_buffer_len;
