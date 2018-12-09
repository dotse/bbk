// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "measurementtask.h"

class InfoTask : public MeasurementTask {
public:
    InfoTask(const std::string &label, const std::string &ticket_string,
             const std::string &hash_key, const HttpHost &server,
             unsigned int no_conn = 1, unsigned int max_conn = 3,
             double duration = 300.0) :
        MeasurementTask(label, ticket_string, server,
                        no_conn, max_conn, duration),
        key(hash_key)
    {
    }
    void newRequest(HttpClientConnection *) override;
    bool requestComplete(HttpClientConnection *) override {
        return false;
    }
    bool wsTextMessage(HttpConnection *,
                       const std::string &msg) override;
    double timerEvent() override;

    // Stop waiting for upload result after at most t seconds:
    void setUploadDeadline(double t) {
        if (t > 0.0)
            upload_deadline = elapsed() + t;
        else
            upload_deadline = t;
    }

    // Stop waiting for measurement ID after at most t seconds:
    void setInfoDeadline(double t) {
        if (t > 0.0)
            info_deadline = elapsed() + t;
        else
            info_deadline = t;
    }
private:
    std::string key;
    double upload_deadline = -1.0;
    double info_deadline = -1.0;
};
