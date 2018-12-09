#pragma once

#include <framework/task.h>

class EchoClientTask : public Task {
public:
    EchoClientTask(const std::string &host, unsigned int port) :
        Task("EchoClientTask"),
        _port(port),
        _hostname(host) {
    }

    void test_succeeded() {
        ++no_succeeded_requests;
        if (no_succeeded_requests == 10)
            setResult("All done!");
    }

    double start() override;

    void connAdded(SocketConnection *) override {
        log() << "connection added";
    }

    void connRemoved(SocketConnection *) override {
        log() << "connection removed";
    }

    double timerEvent() override;

private:
    uint16_t _port;
    std::string _hostname;
    unsigned int no_succeeded_requests = 0;
};
