// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "httptask.h"
#include "cookiemanager.h"
#include "httpclientconnection.h"

/// API for HTTP clients.
class HttpClientTask : public HttpTask {
public:
    HttpClientTask(const std::string &name, const HttpHost &httpserver);

    /// Initiate next request, or ignore to close connection.
    virtual void newRequest(HttpClientConnection *) {
    }

    /// \brief
    /// Called when response headers are fully read and parsed,
    /// except for websocket upgrades, where websocketUpgrade is called instead.
    ///
    /// Override it to send a HTTP request to the server.
    ///
    /// Return false to close the connection, true to continue.
    /// If not overridden, it will always return true.
    virtual bool headerComplete(HttpClientConnection *);

    /// \brief
    /// Called after succesful websocket upgrade.
    ///
    /// Override it to start sending websocket messages to the server.
    ///
    /// Return false to close the connection, true to continue.
    ///
    /// If not overridden, it will always return true.
    virtual bool websocketUpgrade(HttpClientConnection *);

    /// Called when response has been fully read.
    virtual bool requestComplete(HttpClientConnection *) = 0;

    /// \brief
    /// Data has arrived from the server.
    ///
    /// Called whenever some part of the content arrives, *if* you
    /// asked for it by calling doStreamResponse() on conn
    /// in the HttpClientConnection::headerComplete callback.
    virtual void payload(HttpClientConnection *, char *, size_t ) {
    }

    /// \brief
    /// Send POST data, return number of bytes sent.
    ///
    /// You _must_ override this method if you intend to stream post requests.
    /// Typically, you simply do this:
    ///
    ///     return conn->sendData(my_buffer, std::min(my_buffer_len, len));
    virtual size_t doPost(HttpClientConnection *conn, size_t len);

    /// Return server's host name.
    std::string serverHost() const {
        return peer.hostname;
    }

    /// \brief
    /// Extra header lines to be added to outgoing requests.
    ///
    /// If you override this, return zero or more lines ending with "\r\n".
    /// Unless you want to disable cookies, append the return value of
    ///
    ///     cookiemgr->httpHeaderLine(server, uri)
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

    /// \brief
    /// Bind all subsequent client sockets to the given local ip address
    ///
    /// Don't call simultaneously from more than one thread
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
