// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <stdexcept>
#include <map>

#include "httptask.h"
#include "httpserverconnection.h"
#ifdef USE_GNUTLS
class SocketReceiver;
#endif

/// API for HTTP servers.
class WebServerTask : public HttpTask {
public:
    WebServerTask(const std::string &name, const TaskConfig &cfg=TaskConfig());

    double start() override;

    std::string headers(const std::string &code) {
        return "HTTP/1.1 " + code + "\r\n" + fixed_response_headers;
    }

    /// \brief
    /// Return a string that may be inserted into the HTTP response headers.
    ///
    /// Let expiry be 0 for a session cookie, < 0 to delete cookie, otherwise
    /// expiry should be the cookie's max-time in seconds.
    /// If domain is empty, the cookie_domain config parameter will be used.
    /// Example:
    ///
    ///     conn->sendHttpResponse(headers("200 OK",
    ///                                    setCookie("lang", "en", 86400)),
    //                             "text/plain", data)
    std::string setCookie(const std::string &name, const std::string &val,
                          long expiry = 0, const std::string &path = "/",
                          std::string domain = "") const;

    /// When you overload this, call sendHttpResponse and return
    /// WAITING_FOR_REQUEST (or CLOSE), or
    /// call sendHttpResponseHeader and return SENDING_RESPONSE.
    /// In the latter case, your overloaded sendResponseData method will be
    /// called regularly until all is sent.
    virtual HttpState newGetRequest(HttpServerConnection *,
                                    const std::string &) {
        return HttpState::CLOSE;
    }

    /// Overload this if you really have to take control over how the response
    /// is sent. Send up to bytes_left bytes through conn, return _exactly_ the
    /// number of bytes written. To get it correct, you should just do
    ///
    ///     return conn->sendData(...)
    virtual size_t sendResponseData(HttpServerConnection * /* conn */,
                                    size_t /* bytes_left */) {
        // Essentially, your overloaded code should do this:
        // my_buffer = ...
        // size_t sent = conn->sendData(my_buffer,
        //                              std::min(bytes_left, sizeof my_buffer));
        // return sent;
        throw std::logic_error("sendResponseData not implemented");
    }

    virtual HttpState newPostRequest(HttpServerConnection *conn,
                                     const std::string &uri);

    /// \brief
    /// Retrieve part of a post message from client.
    ///
    /// Return true to continue, false to close the connection.
    virtual bool partialPostData(HttpServerConnection *conn,
                                 const char *buffer, size_t len);

    /// \brief
    /// Retrieve last part of a post message from client.
    virtual HttpState lastPostData(HttpServerConnection *conn,
                                   const char *buffer, size_t len);

    /// \brief
    /// Request from client for a websocket upgrade.
    ///
    /// Return true to accept, false to close.
    virtual bool newWsRequest(HttpServerConnection *conn,
                              const std::string &uri);

    /// If you want to send data immediately after a websocket connection
    /// has been established (i.e. before the client has asked for
    /// anything), call conn->notifyWsHandshake() in the newWsRequest method;
    /// then the below method will be called after the handshake:
    virtual void wsHandshakeFinished(HttpServerConnection *conn,
                                     const std::string &uri);

    /// Override this to implement a response to a preflight (OPTIONS) request.
    virtual HttpState preflightRequest(HttpServerConnection * /* conn */,
                                       const std::string & /* uri */) {
        return HttpState::CLOSE;
    }

    void connAdded(SocketConnection *conn) override {
        log() << "new connection id=" << conn->id();
    }

    void connRemoved(SocketConnection *conn) override {
        log() << "dropped connection " << conn->id();
    }

    /// \brief
    /// One or more HTTP response header lines that always should be sent.
    ///
    /// Terminate each line with \r\n.
    void setFixedHeaders(const std::string &hdr) {
        fixed_response_headers = "Server: " + server_name + "\r\n" + hdr;
    }
    SocketConnection *newClient(int fd, const char *ip, uint16_t port,
                                ServerSocket *) final;

    /// Return Access-Control-Allow-Origin header if the conn's current request
    /// originates from domain (or a subdomain to it), otherwise return "".
    static std::string corsHeader(HttpServerConnection *conn,
                                  const std::string &domain);

    std::string webRoot() const {
        return webroot;
    }

    void setWebRoot(const std::string &path) {
        webroot = path;
    }

#ifdef USE_GNUTLS
    void newWorkerChannel(SocketReceiver *srv, unsigned int chan) override;
#endif

protected:
    TaskConfig the_config;
private:
    std::string server_name, fixed_response_headers;
    std::string webroot;
};
