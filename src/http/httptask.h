// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include "../framework/task.h"
#include "httphost.h"

class HttpConnection;

class HttpTask : public Task {
public:
    HttpTask(const std::string &name) :
        Task(name) {
    }

    // Incoming websocket messages. Return false to kill connection.
    virtual bool wsTextMessage(HttpConnection *,
                               const std::string &msg);
    virtual bool wsBinMessage(HttpConnection *,
                              const std::string &msg);
    // Called when headers are read. Return false to kill connection. Unless
    // you call streamWsResponse, the message will be buffered until complete,
    // and then delivered through wsTextMessage / wsBinMessage.
    // If you call conn->streamWsResponse, the message will be streamed through
    // wsBinData / wsTextData.
    virtual bool wsBinHeader(HttpConnection *, size_t ) {
        return true;
    }
    virtual bool wsTextHeader(HttpConnection *, size_t ) {
        return true;
    }

    // Incoming partial websocket messages. Return false to kill connection.
    virtual bool wsBinData(HttpConnection *, const char *, size_t ) {
        return false;
    }
    virtual bool wsTextData(HttpConnection *, const char *, size_t ) {
        return false;
    }

    // If you have called conn->startWsBinStream, sendWsData will be called
    // repetedly (as fast as the network allows) until all data has been sent.
    // Override this to return the number of bytes you sent. You may return 0
    // if you have a temporary error, but it's really bad to keep returning 0.
    // So don't use this feature unless the data to send is readily available.
    // To give up, call conn->abortWsStream() and return 0.
    virtual size_t sendWsData(HttpConnection *conn);
protected:
private:
};
