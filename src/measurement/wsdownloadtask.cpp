// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#include "wsdownloadtask.h"

WsDownloadTask::
WsDownloadTask(const std::string &ticket, const HttpHost &server,
               unsigned int no_conn, unsigned int max_conn,
               double duration, double max_time,
               double tick_s) :
    ProgressTask("download", ticket, server, no_conn, max_conn, duration, max_time),
    tick_duration_s(tick_s>0 ? tick_s : 0.1) {
    dynamic_conn_limit = 0.5 * duration + 0.5;
}

double WsDownloadTask::start() {
    log() << "WsDownloadTask starting";
    checkConnectionCount();
    return tick_duration_s;
}

double WsDownloadTask::timerEvent() {
    double time = elapsed();

    if (time >= timeout_s()) {
        log() << "WsDownloadTask timeout after " << time << " seconds";
        setResult("-1");
        return 0;
    }

    if (time > 0.05) {
        double speed = addOverheadMbps(byteCount(), time);
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

void WsDownloadTask::newRequest(HttpClientConnection *conn) {
        conn->ws_get("/ws?t=" + t());
        //conn->dbgOn();
}

bool WsDownloadTask::nextRequest(HttpConnection *conn) {
    if (size_t size = loadSize()) {
        conn->sendWsMessage("download " + std::to_string(size));
        //log() << "nextRequest " << size;
        return true;
    } else if (soonFinished()) {
        log() << "nextRequest no more";
        return false;
    } else {
        log() << "nextRequest idle";
        // Keep the connection but don't make a new request at this point.
        // Perhaps due to a speed limit.
        //conn->pass();
        return true;
    }
}

bool WsDownloadTask::requestComplete(HttpClientConnection *) {
    return false;
}

bool WsDownloadTask::websocketUpgrade(HttpClientConnection *conn) {
    return nextRequest(conn);
}

bool WsDownloadTask::wsBinMessage(HttpConnection *conn,
                                  const std::string &msg) {
    log() << "Got BIN: " << msg.size();
    notifyBytesLoaded(msg.size());
    return nextRequest(conn);
}

bool WsDownloadTask::wsBinData(HttpConnection *conn, const char *, size_t count) {
    notifyBytesLoaded(count);
    if (conn->wsIncomingBytesLeft())
        return true;
    return nextRequest(conn);
}

bool WsDownloadTask::wsTextMessage(HttpConnection *,
                                   const std::string &msg) {
    log() << "Unexpected ws text message: " << msg;
    return false;
}
