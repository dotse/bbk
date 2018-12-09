#include "webserver.h"
#include <framework/bridgetask.h>


// Messages from the client arrive here:
void WebServer::handleExecution(Task *sender, const std::string &message) {
    dbg_log() << "Event " << message << " from " << sender->label();
    if (BridgeTask *bridge = dynamic_cast<BridgeTask *>(sender)) {
        if (the_bridge && bridge != the_bridge) {
            // We already had a connected bridge and a new one tries to connect.
            // We could allow several simultaneous clients, but normally one is
            // enough.
            log() << "Won't allow more than one client";
            return;
        }

        // Here we can handle all kinds of messages from the client.
        if (message == "client ready") {
            // Client is ready to receive messages from us.
            the_bridge=bridge;
        } else if (message == "quit" ) {
            setResult("Terminate by request from client");
        }
    }
}

void WebServer::taskFinished(Task *task) {
    if (task == the_bridge) {
        log() << "Client connection broken";
        the_bridge = nullptr;
        // We could call  setResult("")  to terminate
        // but will wait for a new client instead.
    }
}

HttpState WebServer::newGetRequest(HttpServerConnection *conn,
                                   const std::string &uri) {
    ++tot_no_requests;
    log() << "URI: " << uri << " #" << tot_no_requests;
    if (the_bridge) {
        std::string msg = "Last request: " + uri + " (request #" +
            std::to_string(tot_no_requests) + ")";
        the_bridge->sendMsgToClient(msg);
    }

    if (uri == "/getTime")
        conn->sendHttpResponse(headers("200 OK"), "text/plain", dateString());
    else if (uri == "/getStats")
        conn->sendHttpResponse(headers("200 OK"), "text/plain",
                               std::to_string(tot_no_requests));
    else if (uri == "/hi")
        conn->sendHttpResponse(headers("200 OK"), "text/plain", "hello");
    else
        conn->sendHttpResponse(headers("404 Not Found"), "text/plain",
                               "unknown service");
    return HttpState::WAITING_FOR_REQUEST;
}
