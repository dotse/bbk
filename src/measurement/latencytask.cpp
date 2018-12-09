#include <algorithm>
#include "../json11/json11.hpp"
#include "latencytask.h"

LatencyTask::LatencyTask(const std::string &ticket, const HttpHost &server) :
    MeasurementTask("httplatency", ticket, server, 1, 15) {
}

void LatencyTask::newRequest(HttpClientConnection *conn) {
    std::string lbl = std::to_string(++serial_no);
    current_request[lbl + " ok"] = timeNow();
    conn->get("/pingpong/" + lbl + "?t=" + t());
}

bool LatencyTask::requestComplete(HttpClientConnection *conn) {
    auto p = current_request.find(conn->contents());
    if (p == current_request.end()) {
        log() << "unexpected response: " << conn->contents();
    } else {
        double latency = secondsSince(p->second);
        log() << "got " << conn->contents() << " after " << latency << " sec";
        samples.push_back(latency);
    }

    if (samples.size() < 12)
        return true;

    if (!terminated()) {
        log() << "Samples: " << json11::Json(samples).dump();
        setResult(calculateLatency(samples));
    }

    return false;
}
