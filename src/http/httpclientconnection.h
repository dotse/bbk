// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <map>
#include <sstream>

#include "httpconnection.h"

class HttpClientTask;

/// \brief
/// HTTP/1.1 client protocol.
///
/// This class implements a small subset of the HTTP/1.1 client protocol.
/// A connection will be made to the given hostname/port.
/// When the connection is ready for a new HTTP request, the owner
/// task's method newRequest(HttpClientConnection *conn) will be called.
/// To use this class, you must derive from HttpClientTask and implement the
/// newRequest method to perform HTTP requests over the HttpClientConnection.
///
/// A call to the owner task's headerComplete will be made when response
/// headers are read. If headerComplete returns false the connection
/// will be dropped.
///
/// Within headerComplete you may call conn->doStreamResponse()
/// if you want the respone to be "streamed" to you, i.e. to be notified
/// (by a call to the owner task's payload method) whenever a part of the
/// response arrives. The default is for this class to buffer the response
/// and only notify the ower task when the complete response has been received.
///
/// A call to the owner task's requestComplete will be made when the complete
/// response has been read. If requestComplete returns false the connection
/// will be dropped. Otherwise, the newRequest will be called again so that
/// a new HTTP request can be made.
/// Within requestComplete, you can call contents() to get the HTTP response
/// unless doStreamResponse() was called before.
class HttpClientConnection : public HttpConnection {
public:
    // TODO: take a HttpHost as a parameter!
    HttpClientConnection(const std::string &label, HttpClientTask *task,
                         const std::string &hostname, uint16_t port,
                         const std::string &http_hostname = std::string(),
                         uint16_t iptype = 0,
                         struct addrinfo *local_addr = nullptr);

    PollState connected() final;
    PollState readData(char *buf, size_t len) final;

    /// For HTTP POST.
    PollState writeData() final;

    /// The owner task should call this in the header_complete
    /// callback if it wants to get partial contents immediately when it
    /// arrives. The default is for this class to buffer the response and
    /// pass it to the owner task when the complete response has arrived.
    void doStreamResponse() {
        buffer_response = false;
    }

    /// The owner task may call the below functions in the header_complete
    /// callback or the response_complete callback.
    unsigned int httpStatus() const {
        return response_http_status;
    }

    /// Return value of first Content-Type header, empty string on failure.
    std::string contentType() const {
        auto p = http_response_headers.find("content-type");
        if (p == http_response_headers.end())
            return std::string();
        else
            return p->second;
    }

    const std::string &rawHttpHeader() const {
        return response_header;
    }

    const std::multimap<std::string, std::string> &responseHeaders() const {
        return http_response_headers;
    }

    size_t remainingContentLength() const {
        return bytes_left_to_read;
    }

    std::string contents() const {
        return buffer;
    }

    /// May be called in the newRequest callback:
    void get(const std::string &url);

    /// May be called in the newRequest callback:
    /// The owner task's doPost method will be called repeatedly until it
    /// returns false, which means there is no more data to post.
    void post(const std::string &url, size_t len);

    // As above, but with first chunk of data available. Using this method
    // may save one call to doPost(), i.e. one less send system call.
    /*
    void post(const std::string &url, size_t len,
              const char *buf, size_t buffer_len, size_t lspace = 0);
    */

    /// May be called in the newRequest callback:
    /// If data is very large, you should use the above method instead.
    void post(const std::string &url, const std::string &data);

    /// May be called in the newRequest callback to open a websocket.
    void ws_get(const std::string &url);

    /// Call in the newRequest callback if you want to keep
    /// the connection but not do any new request at this time.
    void pass() {
        current_request = "pass";
    }
    bool idle() const {
        return current_request.empty() ||
            current_request == "pass";
    }

    static std::string urlencode(const std::string &val);
    static void addUrlPars(std::string &url,
                           const std::map<std::string, std::string> &pars);
    std::string cacheLabel() override;

    void setOwner(Task *new_owner) override;
private:
    std::string real_hostname;
    // A weak pointer to the owner task. Dangerous. However, the EventLoop will
    // kill all connections (i.e. us) before killing the owner task.
    HttpClientTask *owner_task;
    std::string proto_hostname;
    std::string current_request, current_uri;

    // Set if this is a websocket connection:
    std::string websocket_rkey;

    // Used when reading response headers:
    std::string response_header;

    const std::string::size_type max_header_length = 20000;

    bool response_header_complete = false;
    // Set when response headers are parsed:
    bool response_is_chunked;
    bool final_chunk;
    bool buffer_response = true;
    size_t bytes_left_to_read = 0;
    size_t bytes_left_to_post = 0;
    unsigned int response_http_status = 0;
    std::string chunked_info;

    bool parseResponseHeaders();
    std::multimap<std::string, std::string> http_response_headers;
    bool parseChunkedEncodingLength(char *&buf, size_t &len);
};
