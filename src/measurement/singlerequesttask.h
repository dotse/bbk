#pragma once

#include "measurementtask.h"

class SingleRequestTask : public MeasurementTask {
public:
    SingleRequestTask(const std::string &url, const std::string &name,
                      const std::string &ticket, const HttpHost &server);
    void newRequest(HttpClientConnection *conn) override;
    bool requestComplete(HttpClientConnection *conn) override;
private:
    std::string _url;
};
