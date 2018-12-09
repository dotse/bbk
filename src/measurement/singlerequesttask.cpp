#include "singlerequesttask.h"

SingleRequestTask::SingleRequestTask(const std::string &url,
                                     const std::string &name,
                                     const std::string &ticket,
                                     const HttpHost &server) :
    MeasurementTask(name, ticket, server),
    _url(url) {
}

void SingleRequestTask::newRequest(HttpClientConnection *conn) {
    conn->get(_url);
}

bool SingleRequestTask::requestComplete(HttpClientConnection *conn) {
    if (conn->httpStatus() == 200)
        setResult(conn->contents());
    return false;
}
