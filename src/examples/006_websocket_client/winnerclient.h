#pragma once

#include <http/httpclienttask.h>

class WinnerClient : public HttpClientTask {
public:
    WinnerClient(const std::string &host, uint16_t port,
                 const std::string &url);
    double start() override;
    void connRemoved(SocketConnection *conn) override;
    void newRequest(HttpClientConnection *conn) override;
    bool requestComplete(HttpClientConnection *) override;
    bool websocketUpgrade(HttpClientConnection *) override;
    bool wsTextMessage(HttpConnection *conn,
                       const std::string &msg) override;
private:
    std::string _url;
};
