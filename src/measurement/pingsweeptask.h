#pragma once

#include <vector>
#include <deque>
#include "../json11/json11.hpp"
#include "../http/httpclienttask.h"

class PingSweepTask : public HttpClientTask {
public:
    PingSweepTask(const std::string &obj, const HttpHost &server);
    double start() override;
    double timerEvent() override;
    void newRequest(HttpClientConnection *) override;
    bool requestComplete(HttpClientConnection *conn) override;
    std::string cacheLabel() override {
        return std::string(); // Don't store in keep-alive cache
    }
    std::string getResult() {
        if (results.empty())
            return std::string();
        std::string res = results.front();
        results.pop_front();
        return res;
    }
private:
    std::string json_err;
    json11::Json config;
    std::map<std::string, TimePoint> requests;
    std::string best_host;
    std::deque<std::string> results;
    double best_time = 1e6;
};
