#pragma once

#include "measurementtask.h"

class ProgressTask : public MeasurementTask {
public:
    ProgressTask(const std::string &label, const std::string &ticket_string,
                 const HttpHost &server,
                 unsigned int no_conn = 4, unsigned int max_conn = 20,
                 double duration = 10.0, double timeout = 25.0) :
        MeasurementTask(label, ticket_string, server, no_conn, max_conn, timeout),
        byte_count(0),
        current_load_size(50000),
        load_size_check(no_conn + 2),
        current_duration(0.0),
        current_mbps(0.0) {
            if (duration < 2.0)
                tot_duration = 2.0;
            else if (duration > 20.0)
                tot_duration = 20.0;
            else
                tot_duration = duration;
    }

    bool requestComplete(HttpClientConnection *) override;

    void notifyBytesAndDuration(uint64_t count, double duration) {
        double mbps = addOverheadMbps(count, duration);
        doTestProgress(mbps, duration, currentNoConnections());
    }

    void notifyBytesLoaded(size_t n) {
        byte_count += n;
    }

    // If you want to use the threadSendCount/threadRecvCount methods, you
    // should call the below method in your start() method:
    void notifyStarted() {
        thread_send_count = SocketConnection::totBytesSent();
        thread_recv_count = SocketConnection::totBytesReceived();
    }

    void connectionLost() override {
        // We're dead. If more than half the test was completed,
        // we keep the result.
        if (current_duration > tot_duration*0.5)
            setResult(fValue(current_mbps));
        else
            setResult("-1");
    }

    void set_speedlimit(double limit_mbps) {
        speedlimit_mbps = (limit_mbps < 0.5) ? 0.5 : limit_mbps;
        if (speedlimit_mbps < 5.0)
            current_load_size = 5000;
    }

    size_t loadSize();

    double currentDuration() const {
        return current_duration;
    }
    double currentProgress() const {
        return current_duration/tot_duration;
    }
    double currentMbps() const {
        return current_mbps;
    }
protected:
    uint64_t byteCount() {
        return byte_count;
    }

    // Return total number of bytes sent through all sockets in the current
    // thread since last call to notifyStarted():
    uint64_t threadSendCount() {
        return SocketConnection::totBytesSent() - thread_send_count;
    }

    // Return total number of bytes received from all sockets in the current
    // thread since last call to notifyStarted():
    uint64_t threadRecvCount() {
        return SocketConnection::totBytesReceived() - thread_recv_count;
    }

    static double addOverheadMbps(uint64_t n, double s) {
        if (s <= 0.0)
            return 0.0;

        double mbps = n / s * 0.000008;
        if (mbps > 8.0)
            return (mbps * 1.02 + 0.16);
        else
            return mbps * 1.04;
    }
    void doTestProgress(double mbps, double duration, unsigned int no_conn);
    bool soonFinished() const {
        return soon_finished;
    }

private:
    uint64_t byte_count, thread_send_count = 0, thread_recv_count = 0;
    size_t current_load_size;
    unsigned int load_size_check, no_started_loads = 0;
    double tot_duration, current_duration, current_mbps;
    double speedlimit_mbps = 0.0;
    bool soon_finished = false;
};
