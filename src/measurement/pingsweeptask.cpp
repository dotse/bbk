#include "pingsweeptask.h"

PingSweepTask::PingSweepTask(const std::string &cfg,
                             const HttpHost &server) :
    HttpClientTask("pingsweep", server),
    config(json11::Json::parse(cfg, json_err)) {
}

double PingSweepTask::start() {
    unsigned int no_calls = 0;
    for (auto srv : config["servers"].array_items()) {
        //log() << "Ping " << srv["url"].string_value();
        setServer(srv["url"].string_value());
        if (createNewConnection())
            ++no_calls;
    }
    if (no_calls)
        return 5.0;
    setResult("");
    return 0.0;
}

double PingSweepTask::timerEvent() {
    setResult(best_host);
    return 0;
}

void PingSweepTask::newRequest(HttpClientConnection *conn) {
    //log() << "newRequest " << conn->hostname();
    auto p = requests.find(conn->hostname());
    if (p != requests.end())
        return; // Ignore host since we already have made a request to it.

    requests[conn->hostname()] = timeNow();
    conn->get("/pingsweep/req" + std::to_string(requests.size()));
}

bool PingSweepTask::requestComplete(HttpClientConnection *conn) {
    //log() << "requestComplete " << conn->hostname() << " size " << requests.size();
    auto p = requests.find(conn->hostname());
    if (p != requests.end()) {
        double t = 1000 * secondsSince(p->second);
        std::string r = conn->hostname() + " " + std::to_string(t) + " ms";
        log() << r;
        if (results.empty())
            setMessage("ping info");
        results.push_back(r);
        requests.erase(p);
        if (t < best_time) {
            best_time = t;
            best_host = conn->hostname();
        }
    }
    if (requests.empty())
        setResult(best_host);
    return false;
}
