#pragma once

#include "singlerequesttask.h"

class TicketTask : public SingleRequestTask {
public:
    TicketTask(const HttpHost &server, const std::string &key,
               const std::string &host);

    std::string cacheLabel() override {
        return _ticket;
    }

    bool requestComplete(HttpClientConnection *conn) override;
    const std::string &localIp() const {
        return _localIp;
    }
private:
    std::string _ticket, _localIp;
};
