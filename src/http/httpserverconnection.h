// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <map>

#include "httpconnection.h"

class WebServerTask;

enum class HttpState {
    CLOSE,
    WAITING_FOR_REQUEST, SENDING_RESPONSE,
    READING_POST_DATA, WEBSOCKET
};

/// \brief
/// HTTP/1.1 server protocol.
///
/// This class implements a small subset of the HTTP/1.1 server protocol.
/// When a new HTTP GET or POST request is made, the owner task's method
/// newGetRequest or newPostRequest will be called. If a websocket connection
/// is made, the owner task's method newWsRequest will be called.
///
/// To use this class, you must derive from WebServerTask and implement
/// at least one of newGetRequest, newPostRequest, or newWsRequest.
class HttpServerConnection : public HttpConnection {
public:
    HttpServerConnection(const std::string &label, WebServerTask *task, int fd,
                         const char *ip, uint16_t port);
    virtual ~HttpServerConnection() override;
    PollState connected() final {
        return PollState::READ;
    }
    PollState readData(char *buf, size_t len) final;
    //PollState unexpectedData(char *, size_t ) override;
    PollState writeData() override;

    /// Return true if query string in current request has parameter name:
    bool hasQueryPar(const std::string &name) const;

    void eraseQueryPar(const std::string &name) {
        current_query_pars.erase(name);
    }

    /// If query string in current request has parameter "name",
    /// return the value of its first occurrence.
    /// Otherwise return empty string.
    std::string getQueryVal(const std::string &name) const;

    const std::multimap<std::string, std::string> &currentQueryPars() const {
        return current_query_pars;
    }

    std::map<std::string, std::string> currentRequestCookies() const;

    /// Return value of cookie if it exists, otherwise empty string.
    std::string cookieVal(const std::string &name) const;

    /// If HTTP headers in current request has attribute "name",
    /// return the value of its first occurrence.
    /// Otherwise return empty string.
    /// Note: use only lower-case letters in "name", e.g. "content-length".
    std::string getHeaderVal(const std::string &name) const;

    typedef std::multimap<std::string, std::string>::const_iterator Hit;
    std::pair<Hit, Hit> getHeaderVals(const std::string &name) const {
            return http_request_headers.equal_range(name);
    }

    /// Async send response. Return length of what's to be sent.
    size_t sendHttpResponse(const std::string &headers,
                            const std::string &mime,
                            const std::string &contents);

    /// Async send response headers. Return length of what's to be sent:
    size_t sendHttpResponseHeader(const std::string &headers,
                                  const std::string &mime,
                                  size_t content_length);

    /// Async send response headers with the "chunked" encoding.
    /// Return length of what's to be sent.
    size_t sendChunkedResponseHeader(const std::string &headers,
                                     const std::string &mime);

    /// Send chunk. May only be used if we sent chunked headers.
    size_t sendChunk(const std::string &content);

    /// To notify that all chunks have been sent.
    size_t chunkedResponseComplete();

    bool sendingChunkedResponse() const {
        return sending_chunked_response;
    }

#ifdef USE_WEBROOT
    /// Can only be called in the newGetRequest method.
    /// Return true if url is a file that can be sent; then you should call the
    /// sendHttpFileResponse method; otherwise you'd want to send a 404 reply.
    ///
    /// *Warning!* Call might be blocking!
    bool prepareFileResponse(const std::string &url);

    HttpState sendHttpFileResponse(const std::string &headers,
                                   const std::string &mime);

#endif

    const std::string &currentUri() const {
        return current_uri;
    }

    const std::string currentFullUrl() const {
        if (current_query_string.empty())
            return current_uri;
        return current_uri + '?' + current_query_string;
    }

    const std::string &currentQueryString() const {
        return current_query_string;
    }

    /// Number of bytes left of the current HTTP POST.
    size_t remainingPostData() const {
        return remaining_post_data;
    }

    /// The owner task may call this in the newPostRequest handler and return
    /// HttpState::READING_POST_DATA.
    /// In that case, the owner task will not get partialPostData calls,
    /// but only a lastPostData call when the user has posted all data.
    ///
    /// Hint: Check first that remainingPostData() isn't too large.
    void bufferPostData() {
        buffer_post_data = true;
    }

    /// Call this if the owner is to be notified after handshake:
    void notifyWsHandshake() {
        notify_after_ws_handshake = true;
    }

    /// Override this to make sure we're not transfered to an owner Task that
    /// is not a WebServerTask (or subclass).
    void setOwner(Task *new_owner) override;

    /// Current request (first line and headers) in raw form.
    std::string currentRequest() const {
        return current_request;
    }
private:
    // A weak pointer to the owner task. Dangerous. However, the EventLoop will
    // kill all connections (i.e. us) before killing the owner task.
    WebServerTask *owner_task;
    HttpState state = HttpState::WAITING_FOR_REQUEST;
    std::string current_request;
    std::multimap<std::string, std::string> http_request_headers;
    std::string current_method, current_uri;
    std::string current_query_string;
    std::multimap<std::string, std::string> current_query_pars;
    static void get_all_parameters(const char *qpos,
        std::multimap<std::string, std::string> &pars);

    // Will be set when sending a response with known Content-Length:
    size_t remaining_bytes;

#ifdef USE_WEBROOT
    // Will be set if sending_static_file is true:
    int static_file_fd;

    // Will be true when sending a static file:
    bool sending_static_file = false;
#endif

    // Will be true when sending chunked response:
    bool sending_chunked_response = false;

    // If set to true, the owner will be notified after handshake:
    bool notify_after_ws_handshake = false;

    // Will be set in state READING_POST_DATA:
    bool buffer_post_data;
    size_t remaining_post_data;

    bool parse_request();
    PollState got_post_data(const char *buf, size_t len);
    PollState check_buffer();
};
