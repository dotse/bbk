// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "progresstask.h"

class DownloadTask : public ProgressTask {
public:
    DownloadTask(const std::string &ticket, const HttpHost &server,
                 unsigned int no_conn = 10, unsigned int max_conn = 100,
                 double duration = 10.0, double max_time = 25.0,
                 double tick_s = 0.1);
    double start() override;
    double timerEvent() override;
    void newRequest(HttpClientConnection *) override;
    bool headerComplete(HttpClientConnection *) override;
    void payload(HttpClientConnection *, char *, size_t len) override;
private:
    double tick_duration_s;
    double dynamic_conn_limit;
};
