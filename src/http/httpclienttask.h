// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "httptask.h"
#include "cookiemanager.h"
#include "httpclientconnection.h"

class HttpClientTask : public HttpTask {
public:
    HttpClientTask(const std::string &name, const HttpHost &httpserver);
    //double start() override;
    //double timerEvent() override;

    // Initiate next request, or ignore to close connection:
    virtual void newRequest(HttpClientConnection *) {
    }

    // headerComplete is called when response headers are fully read and parsed,
    // except for websocket upgrades, where websocketUpgrade is called instead.
    // Return false to close the connection, true to continue.
    // If you don't override these methods, they will always return true.
    virtual bool headerComplete(HttpClientConnection *);
    virtual bool websocketUpgrade(HttpClientConnection *);

    // Called when response has been fully read:
    virtual bool requestComplete(HttpClientConnection *) = 0;

    // Called whenever some part of the content arrives, *if* you
    // asked for it by calling doStreamResponse() on conn
    // in the headerComplete callback.
    virtual void payload(HttpClientConnection *, char *, size_t ) {
    }

    // Send POST data, return false when all sent.
    // You _must override this method if you intend to stream post requests.
    // Typically, you simply do this:
    //   return conn->sendData(my_buffer, std::min(my_buffer_len, len));
    virtual size_t doPost(HttpClientConnection *conn, size_t len);

    std::string serverHost() const {
        return peer.hostname;
    }

    // Extra header lines to be added to outgoing requests.
    // If you override this, return zero or more lines ending with "\r\n".
    // Unless you want to disable cookies, append the return value of
    //    cookiemgr->httpHeaderLine(server, uri)
    virtual std::string httpHeaderLines(const std::string &uri) const {
        if (!peer.cmgr)
            return user_agent_header;
        return peer.cmgr->httpHeaderLine(peer.hostname, uri) +
            user_agent_header;
    }

    void setCookie(const std::string &header_line, const std::string &uri) {
        if (peer.cmgr)
            peer.cmgr->setCookie(header_line, peer.hostname, uri);
    }
    virtual std::string cacheLabel() {
        return peer.hostname + std::to_string(peer.port);
    }
    void setUserAgentString(const std::string &s) {
        if (s.find_first_of("\r\n") == std::string::npos)
            user_agent_header = "User-Agent: " + s + "\r\n";
    }
    // Bind all subsequent client sockets to the given local ip address
    // Don't call simultaneously from more than one thread
    static bool setLocalAddress(const std::string &addr, uint16_t iptype);
    static const std::string &getLocalAddress() {
        return local_address;
    }
protected:
    bool createNewConnection();

    std::string proxyHost() const {
        return peer.proxyHost;
    }
    uint32_t proxyPort() const {
        return peer.proxyPort;
    }
    CookieManager *cookieMgr() const {
        return peer.cmgr;
    }
    void setServer(const std::string hostname, uint16_t port_number = 80) {
        peer.hostname = hostname;
        peer.port = port_number;
    }
private:
    HttpHost peer;
    std::string user_agent_header;
    std::string real_server;
    static std::string local_address;
    static struct addrinfo *local_ip;
};
