#include "../json11/json11.hpp"
#include "tickettask.h"

TicketTask::TicketTask(const HttpHost &server, const std::string &key,
                       const std::string &host) :
    SingleRequestTask("/ticket?key=" + key + "&host=" + host, "ticket", "",
                      server) {
}

bool TicketTask::requestComplete(HttpClientConnection *conn) {
    if (conn->httpStatus() == 200) {
        std::string err;
        json11::Json ticket_obj = json11::Json::parse(conn->contents(), err);
        if (err.empty()) {
            _ticket = ticket_obj["ticket"].string_value();
            if (!_ticket.empty()) {
                _localIp = conn->localIp();
                setResult(_ticket);
            } else {
                err_log() << "no ticket found";
            }
        } else {
            err_log() << "cannot parse ticket: " << err;
        }

    }
    return false;
}
