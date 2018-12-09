#pragma once

#include <http/webservertask.h>

class BridgeTask;

class WebServer : public WebServerTask {
public:
    WebServer(const std::string &cfg) :
        WebServerTask("WebServer", cfg) {
    }

    HttpState newGetRequest(HttpServerConnection *,
                            const std::string &uri) override;

    void handleExecution(Task *sender, const std::string &message) override;
    void taskFinished(Task *task) override;
private:
    unsigned long tot_no_requests = 0;

    // We will only talk to one client.
    BridgeTask *the_bridge = nullptr;
};
