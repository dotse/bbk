#include <numeric>

#include "measurementtask.h"
#include "../json11/json11.hpp"

#ifdef _WIN32
typedef long ssize_t;
#endif

void MeasurementTask::checkConnectionCount() {
    if (terminated())
        return;

    if (active_connections < no_connections)
        dbg_log() << "checkConnectionCount: act=" << active_connections
                  << " want=" << no_connections
                  << " max=" << max_connections;

    while (active_connections < no_connections && max_connections) {
        --max_connections;
        if (!createNewConnection())
            log() << "couldn't add http connection, " << max_connections
                  << " attempts remain";
    }

    if (no_connections && !active_connections)
        connectionLost();
}

void MeasurementTask::connAdded(SocketConnection *c) {
    ++active_connections;
    log() << "conn added: " << c << " now have " << active_connections;
}

void MeasurementTask::connRemoved(SocketConnection *c) {
    --active_connections;
    log() << "conn removed: " << c << " now have " << active_connections;
    if (!terminated()) {
        checkConnectionCount();
    }
}

std::string MeasurementTask::json_obj(const std::string &attr,
                                      const std::string &value) {
    json11::Json obj = json11::Json::object {
        { attr, value },
    };
    return obj.dump();
}

std::string MeasurementTask::calculateLatency(std::vector<double> &samples) {
    if (samples.size() < 5)
        return std::string(); // Too few samples

    // Keep the best 60%
    std::sort(samples.begin(), samples.end());
    ssize_t n = static_cast<ssize_t>(samples.size() * 3 / 5);

    double latency_sum = std::accumulate(samples.cbegin(),
                                         samples.cbegin()+n, 0.0);
    return fValue(1000.0 * latency_sum / static_cast<double>(n));
}
