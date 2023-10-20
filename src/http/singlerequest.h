// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "httpclienttask.h"

class SingleRequest : public HttpClientTask {
public:
    // GET url
    SingleRequest(const std::string &name, const HttpHost &host,
                  const std::string &url, double timeout = 0.0) :
        HttpClientTask(name, host), _url(url), _timeout(timeout) {
    }
    // POST post_data to url.
    SingleRequest(const std::string &name, const HttpHost &host,
                  const std::string &url, const std::string &post_data,
                  double timeout = 0.0) :
        HttpClientTask(name, host), _url(url),
        _post_data(post_data), _timeout(timeout) {
    }

    double start() override;

    double timerEvent() override;

    void newRequest(HttpClientConnection *conn) override {
        if (_post_data.empty())
            conn->get(_url);
        else
            conn->post(_url, _post_data);
    }

    bool requestComplete(HttpClientConnection *conn) override;

    void connRemoved(SocketConnection *) override {
        if (!terminated())
            setError("SingleRequest Failure");
    }

    unsigned int httpStatus() const {
        return _status;
    }
private:
    std::string _url;
    std::string _post_data;
    double _timeout;
    unsigned int _status = 0;
};
