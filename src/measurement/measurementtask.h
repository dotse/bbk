#pragma once

#include "../http/httpclienttask.h"
#include "../http/httphost.h"
#include "defs.h"
#include <clocale>
#include <sstream>

class MeasurementTask : public HttpClientTask {
public:
    MeasurementTask(const std::string &name, const std::string &ticket,
                    const HttpHost &httpserver,
                    unsigned int no_conn = 1, unsigned int max_conn = 3,
                    double timeout = 25.0) :
    HttpClientTask(name, httpserver),
        no_connections(no_conn),
        active_connections(0),
        max_connections(max_conn),
        timeout_sec(timeout),
        ticket_string(ticket) {
        setUserAgentString(measurement::appName + " " + measurement::appVersion);
    }

    std::string t() {
        return ticket_string;
    }

    // Only use keep-alive within the same measurement:
    std::string cacheLabel() override {
        return ticket_string;
    }

    // If you override this, you must call checkConnectionCount().
    // Return value is number of seconds until timerEvent
    // should be called, <= 0 for never.
    double start() override {
        checkConnectionCount();
        return timeout_sec;
    }

    // If you override timerEvent(), you'll have to check for timeout.
    /*
    double timerEvent() override {
        if (timeout())
            setResult("");
        return 0;
    }
    */

    // Will be called if checkConnectionCount fails.
    // Default is to terminate task with an empty result.
    virtual void connectionLost() {
        log() << "connectionLost()";
        setResult("");
    }

    // Returns a JSON object with a single attribute-value pair
    static std::string json_obj(const std::string &attr,
                                const std::string &value);

    // Return floating point value as string.
    // We don't want to use to_string since it's locale dependent,
    // which may break JSON.
    static std::string fValue(double x) {
        std::ostringstream s;
        s.imbue(std::locale("C"));
        s << x;
        return s.str();
    }

    static std::string calculateLatency(std::vector<double> &samples);
protected:
    // Tries to make sure we have at least no_connections simultaneous
    // HTTP connections. Will not try more than max_connection times.
    // Will call connectionLost on fatal failure.
    void checkConnectionCount();

    // Will not kill already created connections.
    void setNoConnections(unsigned int no) {
        no_connections = no;
        checkConnectionCount();
    }

    unsigned int getNoConnections() const {
        return no_connections;
    }

    unsigned int currentNoConnections() const {
        return active_connections;
    }

    void noMoreConnections() {
        max_connections = 0;
    }

    bool timeout() {
        if (timeout_sec < 0)
            return false;
        if (elapsed() < timeout_sec)
            return false;
        log() << "Task timeout.";
        return true;
    }

    double timeout_s() {
        return timeout_sec;
    }
private:
    void connAdded(SocketConnection *) final;
    void connRemoved(SocketConnection *) final;

    unsigned int no_connections, active_connections, max_connections;
    double timeout_sec;
    std::string ticket_string;
};
