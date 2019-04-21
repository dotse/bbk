#pragma once

#include <http/webservertask.h>
#include <string>
#include <map>

struct Info {
    std::string name;
    unsigned long score;
};

class Winner : public WebServerTask {
public:
    Winner(const std::string &cfg) :
        WebServerTask("Winner", cfg) {
    }

    // Client want's to open a websocket connection on uri.
    // Return true to accept, false to close the connection.
    bool newWsRequest(HttpServerConnection *conn,
                      const std::string &uri) override;

    // When a text message is sent from the client, we'll be notified here:
    bool wsTextMessage(HttpConnection *conn,
                       const std::string &msg) override;

    void connRemoved(SocketConnection *conn) override;
 private:
    std::map<HttpConnection *, Info> leader;
};
