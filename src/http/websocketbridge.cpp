// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#include "../json11/json11.hpp"
#include "websocketbridge.h"
#include "../http/httpclientconnection.h"

#if defined(__APPLE__)
#include <CoreFoundation/CFBundle.h>
#include <ApplicationServices/ApplicationServices.h>
#endif

class WSBlistener : public WebServerTask {
public:
    WSBlistener(Task *bridge, const TaskConfig &cfg);

    bool newWsRequest(HttpServerConnection *conn,
                      const std::string &uri) override;

    bool wsTextMessage(HttpConnection *conn,
                       const std::string &msg) override;
    void sendMsgToClient(const std::string &msg) {
        dbg_log() << "Send to client: " << msg;
        if (client)
            client->sendWsMessage(msg);
    }

    void connAdded(SocketConnection *s) override;
    void serverAdded(ServerSocket *s) override;
    void connRemoved(SocketConnection *s) override;
    void serverRemoved(ServerSocket *s) override;

    uint16_t listenPort() const {
        if (server)
            return server->port();
        return 0;
    }

    void run_browser(const std::string &url);

    void processFinished(int pid, int wstatus) override;

    // Returns empty string unless ready:
    std::string url() const {
        return connect_url;
    }
private:
    void notify_url() {
        std::cerr << "\nOPEN URL: " << connect_url << std::endl;
    }
    Task *parent;
    int external_browser_pid = 0;
    std::string connect_url;
    HttpServerConnection *client = nullptr;
    ServerSocket *server = nullptr;
    TaskConfig config;
};

WebsocketBridge::WebsocketBridge(Task *agent, const TaskConfig &cfg) :
    BridgeTask(agent),
    listen_task(new WSBlistener(this, cfg)) {
    killChildTaskWhenFinished();
}

WebsocketBridge::~WebsocketBridge() {
}

double WebsocketBridge::start() {
    dbg_log() << "WebsocketBridge::start()";
    double x = BridgeTask::start();
    addNewTask(listen_task, this);
    listen_port.store(listen_task->listenPort());
    listen_task->startObserving(this);
    dbg_log() << "Listening on port " << listen_port.load();
    return x;
}

void WebsocketBridge::handleExecution(Task *sender, const std::string &msg) {
    log() << "Got msg: " << msg;
    if (sender == listen_task)
        sendMsgToAgent(msg);
}

void WebsocketBridge::taskFinished(Task *task) {
    if (task == listen_task) {
        log() << "Listen task dead";
        // We will not be able to talk to the client anymore.
        listen_task = nullptr;
        setResult("");
    } else {
        log() << "Agent task dead";
        BridgeTask::taskFinished(task);
    }
}

std::string WebsocketBridge::url() const {
    if (!listen_task || !listen_port.load())
        return std::string();
    return listen_task->url();
}

WSBlistener::WSBlistener(Task *bridge, const TaskConfig &cfg) :
    WebServerTask("WSB", "listen " + cfg.value("listen") +
                  " " + cfg.value("listen_addr")),
    parent(bridge),
    config(cfg) {
}

void WebsocketBridge::sendMsgToClient(const std::string &msg) {
    log() << "Send msg " << msg;
    if (listen_task)
        listen_task->sendMsgToClient(msg);
}

void WSBlistener::run_browser(const std::string &url) {
#if defined(__APPLE__)
    CFURLRef url2 = CFURLCreateWithBytes(nullptr, (UInt8 *)url.c_str(),
                                         url.size(), kCFStringEncodingASCII,
                                         nullptr);
    LSOpenCFURLRef(url2, 0);
    CFRelease(url2);
#elif defined(_WIN32)
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
#else
    const char *const argv[] = { "xdg-open", url.c_str(), nullptr };
    external_browser_pid = runProcess(argv);
#endif
}

void WSBlistener::processFinished(int pid, int wstatus) {
    log() << "Process " << pid << " finished, status: " << wstatus;
    if (pid == external_browser_pid && wstatus)
        notify_url();
}

bool WSBlistener::newWsRequest(HttpServerConnection *conn,
                               const std::string &uri) {
    if (client) {
        log() << "error: already have a client";
        return false;
    }

    if (uri != "/wsbridge") {
        log() << "bad uri: " << uri;
        return false;
    }
    if (config.value("listen_pw") != conn->getQueryVal("pwd")) {
        log() << "bad password";
        return false;
    }

    client = conn;

    // Accept only one client:
    if (server)
        server->stopListening();

    return true;
}

bool WSBlistener::wsTextMessage(HttpConnection *,
                                const std::string &msg) {
    log() << "Got msg " << msg;
    executeHandler(parent, msg);
    return true;
}

void WSBlistener::connAdded(SocketConnection *) {
    log() << "WebsocketBridge client connected";
}

void WSBlistener::serverAdded(ServerSocket *socket) {
    log() << "WebsocketBridge listening on port " << socket->port();
    server = socket;
    connect_url = config.value("url");
    log() << "Connect URL: " << connect_url;
    std::map<std::string, std::string> pars;
    pars["bridge"] = "ws";
    pars["port"] = std::to_string(socket->port());
    if (socket->hostname() != "127.0.0.1")
        pars["ip"] = socket->hostname();
    if (!config.value("listen_pw").empty())
        pars["pwd"] = config.value("listen_pw");
    HttpClientConnection::addUrlPars(connect_url, pars);
    log() << "Browser: " << connect_url;
    if (config.value("browser") == "1") {
        run_browser(connect_url);
    } else if (config.value("browser") == "0") {
        notify_url();
    }
}

void WSBlistener::connRemoved(SocketConnection *s) {
    if (dynamic_cast<HttpServerConnection *>(s) == client) {
        log() << "WebsocketBridge client disconnected";
        setResult("");
    }
}

void WSBlistener::serverRemoved(ServerSocket *socket) {
    log() << "WebsocketBridge listening port closed: " << socket->port();
    server = nullptr;
}
