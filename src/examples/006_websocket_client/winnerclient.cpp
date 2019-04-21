#include "winnerclient.h"

WinnerClient::WinnerClient(const std::string &host, uint16_t port,
                           const std::string &url) :
    HttpClientTask("WinnerClient", HttpHost(host, port)),
    _url(url) {
}

double WinnerClient::start() {
    log() << "starting";
    if (!createNewConnection()) {
        log() << "Cannot connect to server\n";
        setResult("error");
    }
    return 10;
}

void WinnerClient::connRemoved(SocketConnection *) {
    if (!terminated()) {
        log() << "Lost connection";
        setResult("error");
    }
}

void WinnerClient::newRequest(HttpClientConnection *conn) {
    conn->ws_get(_url);
}

bool WinnerClient::requestComplete(HttpClientConnection *) {
    // Server ignored websocket upgrade request
    return false;
}

bool WinnerClient::websocketUpgrade(HttpClientConnection *conn) {
    log() << "connected";
    conn->sendWsMessage("Bill 12");
    conn->sendWsMessage("Steve 19");
    conn->sendWsMessage("Linus 33");
    conn->sendWsMessage("Ken 27");
    conn->sendWsMessage("winner");
    return true;
}

bool WinnerClient::wsTextMessage(HttpConnection *,
                                 const std::string &msg) {
    log() << "Got " << msg;
    setResult(msg);
    return false;
}
