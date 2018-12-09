#pragma once

#include "progresstask.h"

class UploadTask : public ProgressTask {
public:
    UploadTask(const std::string &ticket, const HttpHost &server,
               unsigned int no_conn = 4, unsigned int max_conn = 20,
               double duration = 10.0, double max_time = 25.0,
               double tick_s = 0.1);
    double start() override;
    double timerEvent() override;
    void newRequest(HttpClientConnection *conn) override;
    bool requestComplete(HttpClientConnection *conn) override;
    size_t doPost(HttpClientConnection *conn, size_t len) override;
private:
    double tick_duration_s;
    static const size_t post_buffer_len = 131072;
    std::map<HttpClientConnection *, size_t> post_size;
};
