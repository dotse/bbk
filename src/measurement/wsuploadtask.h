// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "progresstask.h"

class WsUploadTask : public ProgressTask {
public:
    WsUploadTask(const std::string &ticket, const HttpHost &server,
               unsigned int no_conn = 4, unsigned int max_conn = 20,
               double duration = 10.0, double max_time = 25.0,
               double tick_s = 0.1);
    double start() override;
    double timerEvent() override;
    void newRequest(HttpClientConnection *conn) override;
    bool websocketUpgrade(HttpClientConnection *conn) override;
    bool wsBinMessage(HttpConnection *conn,
                       const std::string &msg) override;
    bool wsTextMessage(HttpConnection *conn,
                       const std::string &msg) override;
    size_t sendWsData(HttpConnection *conn) override;
private:
    bool nextRequest(HttpConnection *conn);
    double tick_duration_s;
    static const size_t post_buffer_len = 131072;
    std::map<HttpClientConnection *, size_t> bytes_left_to_post, post_size;
};
