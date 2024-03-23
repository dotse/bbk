// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#include "downloadtask.h"

DownloadTask::DownloadTask(const std::string &ticket, const HttpHost &server,
                           unsigned int no_conn, unsigned int max_conn,
                           double duration, double max_time, double tick_s) :
    ProgressTask("download", ticket, server, no_conn, max_conn, duration, max_time),
    tick_duration_s(tick_s>0 ? tick_s : 0.1) {
    dynamic_conn_limit = 0.5 * duration + 0.5;
}

double DownloadTask::start() {
    log() << "DownloadTask starting";
    notifyStarted();
    checkConnectionCount();
    return tick_duration_s;
}

double DownloadTask::timerEvent() {
    double time = elapsed();

    if (time >= timeout_s()) {
        log() << "DownloadTask timeout after " << time << " seconds";
        setResult("-1");
        return 0;
    }

    if (time > 0.05) {
        // log() << "Check: " << byteCount() << " and " << threadRecvCount();
        double speed = addOverheadMbps(threadRecvCount(), time);
        if (time < dynamic_conn_limit && speed >= 50.0) {
            unsigned int no_conn = (speed < 250.0) ?
                ((speed < 100) ? 12 : 24) :
                ((speed < 500) ? 32 : 48);
            if (no_conn < currentNoConnections())
                no_conn = currentNoConnections();
            doTestProgress(speed, time, no_conn);
            setNoConnections(no_conn);
        } else {
            doTestProgress(speed, time, currentNoConnections());
        }
    }
    return tick_duration_s;
}

void DownloadTask::newRequest(HttpClientConnection *conn) {
    if (size_t size = loadSize()) {
        conn->get("/bigfile.bin?t=" + t() + "&len=" + std::to_string(size));
    } else if (soonFinished()) {
        // Delete the Connection object but let socket go to keep-alive cache.
    } else {
        // Keep the connection but don't make a new request at this point.
        // Perhaps due to a speed limit.
        conn->pass();
    }
}

bool DownloadTask::headerComplete(HttpClientConnection *conn) {
    conn->doStreamResponse();
    notifyBytesLoaded(conn->rawHttpHeader().size());
    return true;
}

void DownloadTask::payload(HttpClientConnection *, char *, size_t len) {
    notifyBytesLoaded(len);
}
