// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#include <sstream>
#include <fstream>

#include "../framework/serversocket.h"
#include "webservertask.h"
#include "httpserverconnection.h"

WebServerTask::WebServerTask(const std::string &name, const TaskConfig &cfg) :
    HttpTask(name),
    the_config(cfg),
    server_name(name) {
    setFixedHeaders("");
}

double WebServerTask::start() {
    log() << "WebServerTask::start()";
    parseListen(the_config, "Web server");
    webroot = the_config.value("webroot");
    return 0;
}

std::string WebServerTask::setCookie(const std::string &name,
                                     const std::string &val,
                                     long expiry,
                                     const std::string &path,
                                     std::string domain) const {
    std::string line = "Set-Cookie: " + name + "=";
    if (expiry >= 0)
        line += val;

    if (!expiry)  // Session cookie
        return line + "\r\n";

    if (domain.empty()) {
        domain = the_config.value("cookie_domain");
        if (domain.empty())
            return "";
    }

    std::string date = dateString2(expiry < 0 ? 1 : time(nullptr) + expiry);

    // Set-Cookie: previous_isp=23; expires=Wed, 07-Jun-2017 11:34:59 GMT;
    // path=/; domain=.bredbandskollen.se
    return line + "; expires=" + date + " GMT; path=" + path +
        "; domain=" + domain + "\r\n";
}

SocketConnection *WebServerTask::
newClient(int fd, const char *ip, uint16_t port, ServerSocket *) {
    return new HttpServerConnection("Web client", this, fd, ip, port);
}

HttpState WebServerTask::newPostRequest(HttpServerConnection *,
                                        const std::string &uri) {
    log() << "POST: " << uri;
    return HttpState::READING_POST_DATA;
}

// Return true to continue, false to close the connection.
bool WebServerTask::partialPostData(HttpServerConnection *,
                                    const char *, size_t ) {
    return true;
}

HttpState WebServerTask::lastPostData(HttpServerConnection *,
                                      const char *, size_t ) {
    return HttpState::CLOSE;
}

bool WebServerTask::newWsRequest(HttpServerConnection *,
                                 const std::string &uri) {
    log() << "WS url: " << uri;
    return true;
}

void WebServerTask::wsHandshakeFinished(HttpServerConnection *,
                                        const std::string &) {
}

// Origin: http://www.mydomain.com:9001
// Origin: https://www.mydomain.com
// domain should be "mydomain.com" or "www.mydomain.com"
std::string WebServerTask::corsHeader(HttpServerConnection *conn,
                                      const std::string &domain) {
    if (domain.empty())
        return "Access-Control-Allow-Origin: *\r\n";
    std::string origin = conn->getHeaderVal("origin");
    auto pos = origin.find(domain);
    if (pos == std::string::npos)
        return std::string();

    if (pos && origin[pos-1] != '/' && origin[pos-1] != '.')
        return std::string();

    if (origin.size() != pos + domain.size() &&
        origin[pos + domain.size()] != ':')
        return std::string();

    return "Access-Control-Allow-Origin: " + origin + "\r\n";
}

#ifdef USE_GNUTLS
#include "../framework/socketreceiver.h"

void WebServerTask::newWorkerChannel(SocketReceiver *srv, unsigned int chan) {
    std::string cfg = the_config.value("channel" + std::to_string(chan));
    if (cfg.empty())
        return;
    std::istringstream s(cfg);
    std::string tls, cert, key, password;
    s >> tls >> cert >> key >> password;
    if (tls != "tls")
        return;
    if (key.empty() || !tlsSetKey(srv, cert, key, password)) {
        err_log() << "cannot load TLS certificate " << cert;
        setError("cannot use TLS certificate");
    }
}
#endif
