// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#include <fstream>

#if defined(RUN_SERVER)
#include "../server/measurementserver.h"
#endif

#include "../http/httphost.h"
#include "../http/websocketbridge.h"
#include "../measurement/measurementagent.h"
#include "utils.h"
#include "cliclient.h"

int main(int argc, char *argv[]) {

    // Options for the measurement agent and the client (user interface):
    TaskConfig agent_cfg, config;

    if (!parseArgs(argc, argv, config, agent_cfg))
        return 1;

    std::ofstream log_file;
    config.openlog(log_file);
    if (!log_file) {
        std::cerr << "cannot write to log file" << std::endl;
        return 1;
    }

    EventLoop loop;

#if defined(RUN_SERVER)
    if (config.value("run_server") == "1") {
        std::string srv_cfg = "listen " + config.value("listen");
        if (!config.value("Measure.LocalAddress").empty())
            (srv_cfg += ' ') += config.value("Measure.LocalAddress");
        loop.addTask(new MeasurementServer(srv_cfg));
        loop.runUntilComplete();
        return 0;
    }
#endif

    CookieFile cf(config.value("config_file"));
    HttpHost webserver(agent_cfg.value("Measure.Webserver"), 80, "", 0, &cf);
    MeasurementAgent *agent = new MeasurementAgent(agent_cfg, webserver);
    CliClient client(config);
    if (config.value("listen").empty()) {
        loop.addTask(new SynchronousBridge(agent, &client));
    } else {
        loop.addTask(new WebsocketBridge(agent, config));
    }

    loop.runUntilComplete();
    return 0;
}
