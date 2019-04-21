#include "winner.h"

bool Winner::newWsRequest(HttpServerConnection *,
                  const std::string &uri) {
    log() << "websocket request to " << uri;

    if (uri == "/winner")
        return true;
    else
        return false;
}

bool Winner::wsTextMessage(HttpConnection *conn,
                           const std::string &msg) {
    log() << "Got: " << msg;
    auto p = leader.find(conn);
    if (msg == "winner") {
        if (p != leader.end()) {
            log() << "Sending " << p->second.name;
            conn->sendWsMessage(p->second.name);
        }
        return true;
    }
    auto pos = msg.find_last_of(" ");
    if (pos == std::string::npos || !pos || pos+1 == msg.size())
        return true;
    if (msg.find_last_not_of("0123456789") != pos)
        return true;
    auto score = std::stoul(msg.substr(pos+1));
    if (p == leader.end()) {
        leader[conn] = { msg.substr(0, pos), score };
    } else if (score > p->second.score) {
        p->second.score = score;
        p->second.name = msg.substr(0, pos);
    } else if (score == p->second.score) {
        (p->second.name += ", ") += msg.substr(0, pos);
    }
    return true;
}

void Winner::connRemoved(SocketConnection *conn) {
    leader.erase(dynamic_cast<HttpConnection *>(conn));
}
