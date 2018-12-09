#pragma once

#include "progresstask.h"

class UploadInfoTask : public ProgressTask {
public:
    UploadInfoTask(const std::string &ticket, const HttpHost &server,
                   double duration = 10.0, double max_time = 25.0);
    double start() override;
    void newRequest(HttpClientConnection *) override;
    bool headerComplete(HttpClientConnection *) override;
    void payload(HttpClientConnection *, char *, size_t ) override;
private:
    std::string buffer;
};
