// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

// Perform a standard speed test, measuring latency, download speed, and upload
// speed.

#include "../json11/json11.hpp"
#include "../framework/task.h"
#include "../http/httphost.h"

class MeasurementAgent;
class InfoTask;

class SpeedTest : public Task {
public:
    SpeedTest(MeasurementAgent *agent, const HttpHost &mserver,
              const std::map<std::string, std::string> &report_data);
    double start() override;
    void taskMessage(Task *task) override;
    void taskFinished(Task *task) override;
    void uploadComplete();
    void doSaveReport(const json11::Json &args = json11::Json::object());

private:
    MeasurementAgent *the_agent = nullptr;
    InfoTask *info_task = nullptr;
    HttpHost mserv;
    std::map<std::string, std::string> report;
    std::string tstr;
    double upload_duration, download_duration;
    std::string server_upload_speed, local_upload_speed;

    bool report_sent_to_server = false;
    bool websocket_works = false;
    // Normally, the server should calculate the latency result.
    // If that failed and we have a locally calculated latency result,
    // this will be set to true:
    bool local_latency = false;

    double speed_limit = 0.0;
    unsigned int initial_no_dconn = 10, initial_no_uconn = 4;
    unsigned int max_no_dconn = 100, max_no_uconn = 100;
};
