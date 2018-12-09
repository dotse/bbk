// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "measurementtask.h"

class RpingTask : public MeasurementTask {
public:
    RpingTask(const std::string &label, const std::string &ticket_string,
              const HttpHost &server,
              unsigned int no_conn = 1, unsigned int max_conn = 20,
              double duration = 25.0) :
        MeasurementTask(label, ticket_string, server,
                        no_conn, max_conn, duration)
    {
    }
    void newRequest(HttpClientConnection *) override;
    bool requestComplete(HttpClientConnection *) override {
        return false;
    }
    bool websocketUpgrade(HttpClientConnection *) override;
    bool wsTextMessage(HttpConnection *conn,
                       const std::string &msg) override;
    void calc_local_result();
private:
    std::vector<double> samples;
    unsigned int max_roundtrips = 100;
    bool sent_challenge = false;
};
