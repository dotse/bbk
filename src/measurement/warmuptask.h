#pragma once

#include "measurementtask.h"

class WarmUpTask : public MeasurementTask {
public:
    WarmUpTask(const std::string &ticket, const HttpHost &server,
               unsigned int no_conn, double timeout) :
        MeasurementTask("warmup", ticket, server, no_conn, no_conn, timeout),
        url("/pingpong/warmup?t=" + ticket) {
    }
    void newRequest(HttpClientConnection *conn) override;
    bool requestComplete(HttpClientConnection *conn) override;
private:
    std::string url;
};
