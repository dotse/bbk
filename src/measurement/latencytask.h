#pragma once

#include <vector>
#include "measurementtask.h"

class LatencyTask : public MeasurementTask {
public:
    LatencyTask(const std::string &ticket, const HttpHost &server);
    void newRequest(HttpClientConnection *) override;
    bool requestComplete(HttpClientConnection *conn) override;
private:
    std::vector<double> samples;
    // Maps expected response to start time of request:
    std::map<std::string, TimePoint> current_request;
    unsigned int serial_no = static_cast<unsigned int>(rand());
};
