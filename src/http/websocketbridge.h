// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <deque>
#include <atomic>

#include "../framework/bridgetask.h"
#include "../http/webservertask.h"
#include "../framework/serversocket.h"

class WSBlistener;

// Open a web server, wait for one (and only one) client to open a websocket
// connection to the server. The websocket connection will then be used to
// pass text messages between the agent and the client.
// The client must connect to the URL /wsbridge
//
// Config parameter - ValIT
//
//   listen - port number for the web server (0 to use a random port)
//   listen_addr - IP number to listen on, e.g. 127.0.0.1 (or 0.0.0.0 for any)
//   listen_pw - (optional) password; cilent must supply the password as value
//               of URL parameter pwd when connecting to /wsbridge
//   browser - (optional) value 0, 1, 2, or 3. If 0, write an URL to stderr;
//             user may open the URL in a browser to load a web interface.
//             If 1, an attempt to fire up the web interface in a new browser
//             tab will be made. If that fails, write the URL to stderr.
//             If 2, only generate the URL to the web interface. The client
//             program must open the URL in a web view.
//             If 3, generate the URL to the next generation web interface. The
//             client program must open the URL in a web view. The web interface
//             uses HTTPS, but must connect using an unencrypted web socket.
//             Note that most webview components will refuse to do so.
//   url - (optional) domain name to the server containing the legacy web
//         interface; ignored if value of "browser" is "3".

class WebsocketBridge : public BridgeTask {
public:
    WebsocketBridge(Task *agent, const TaskConfig &cfg);

    virtual ~WebsocketBridge() override;

    double start() override;

    void sendMsgToClient(const std::string &msg) override;

    void handleExecution(Task *sender, const std::string &msg) override;

    void taskFinished(Task *task) override;

    // Returns empty string unless ready. Thread safe.
    std::string url() const;
    std::string port() const;

    // Return true if client is connected:
    bool clientConnected() const;

private:
    WSBlistener *listen_task = nullptr;
    std::atomic<uint16_t> listen_port;


};
