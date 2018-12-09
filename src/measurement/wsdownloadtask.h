// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "progresstask.h"

class WsDownloadTask : public ProgressTask {
public:
    WsDownloadTask(const std::string &ticket, const HttpHost &server,
                   unsigned int no_conn = 10, unsigned int max_conn = 100,
                   double duration = 10.0, double max_time = 25.0,
                   double tick_s = 0.1);
    double start() override;
    double timerEvent() override;
    void newRequest(HttpClientConnection *) override;
    bool requestComplete(HttpClientConnection *) override;
    bool websocketUpgrade(HttpClientConnection *) override;
    bool wsBinMessage(HttpConnection *conn,
                       const std::string &msg) override;
    bool wsTextMessage(HttpConnection *conn,
                       const std::string &msg) override;
    bool wsBinHeader(HttpConnection *conn, size_t ) override {
        conn->streamWsResponse();
        return true;
    }
    bool wsBinData(HttpConnection *conn, const char *, size_t count) override;
private:
    bool nextRequest(HttpConnection *conn);
    double tick_duration_s;
    double dynamic_conn_limit;
};
