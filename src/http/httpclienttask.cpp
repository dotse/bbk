#include <stdexcept>

#include "httpclienttask.h"

HttpClientTask::HttpClientTask(const std::string &name,
                               const HttpHost &httpserver) :
    HttpTask(name),
    peer(httpserver) {
    if (!peer.proxyHost.empty()) {
        real_server = peer.hostname + ":" + std::to_string(peer.port);
    }
}

bool HttpClientTask::createNewConnection() {
    HttpClientConnection *conn = real_server.empty() ?
        new HttpClientConnection(label(), this, peer.hostname, peer.port, "",
                                 peer.iptype, local_ip) :
        new HttpClientConnection(label(), this,
                                 peer.proxyHost, peer.proxyPort,
                                 real_server, peer.iptype, local_ip);
#ifdef USE_GNUTLS
    if (peer.is_tls)
        conn->enableTLS();
#endif
    return addConnection(conn);
}

bool HttpClientTask::headerComplete(HttpClientConnection *) {
    return true;
}

size_t HttpClientTask::doPost(HttpClientConnection *, size_t ) {
    throw std::runtime_error("doPost method not implemented");
}

bool HttpClientTask::websocketUpgrade(HttpClientConnection *) {
    return true;
}

std::string HttpClientTask::local_address;
struct addrinfo *HttpClientTask::local_ip = nullptr;

bool HttpClientTask::setLocalAddress(const std::string &addr, uint16_t iptype) {
    local_address = addr;
    if (local_address.empty()) {
        local_ip = nullptr;
        return true;
    }
    Socket sobj("temp", nullptr, local_address, 0);
    local_ip = sobj.getAddressInfo(iptype);
    return (local_ip != nullptr);
}
