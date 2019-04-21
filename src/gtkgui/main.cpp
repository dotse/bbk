// Copyright (c) 2019 Internetstiftelsen
// Written by Göran Andersson <initgoran@gmail.com>

#include "gtkclient.h"

#include "../measurement/measurementagent.h"
#include "../cli/utils.h"
#include "../framework/unixdomainbridge.h"

namespace {
    void runAgent(BridgeTask *bridge, const TaskConfig &cfg,
                  const std::string &cfgfile, const std::string &logfile) {
        std::ofstream log_file;
        if (logfile != "-") {
            log_file.open(logfile);
            Logger::setLogFile(log_file);
        }
        CookieFile cf(cfgfile);
        HttpHost webserver(cfg.value("Measure.Webserver"), 80, "", 0, &cf);
        EventLoop loop;
        bridge->setAgent(new MeasurementAgent(cfg, webserver));
        loop.addTask(bridge);
        loop.runUntilComplete();
    }
}

int main(int argc, char *argv[]) {

    TaskConfig agent_cfg, config;
    if (!parseArgs(argc, argv, config, agent_cfg))
        return 1;
    agent_cfg.set("Measure.Webserver", "beta4.bredbandskollen.se");
    agent_cfg.set("Measure.AutoSaveReport", "true");

    std::ofstream log_file;
    if (config.value("logfile") != "-") {
        log_file.open(config.value("app_dir") + "gtk_last_log");
        Logger::setLogFile(log_file);
    }

    UnixDomainBridge *bridge = new UnixDomainBridge();
    int client_socket = bridge->getClientSocket();
    if (!client_socket)
        exit(1);

    auto client = new GtkClient(config, client_socket);

    std::thread agent_thread(runAgent, bridge, agent_cfg,
                             config.value("config_file"), config.value("logfile"));

    GtkApplication *app = gtk_application_new("bbk.iis.se",
                                              G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(GtkClient::activate), client);
    int status = g_application_run(G_APPLICATION(app), 0, nullptr);
    g_object_unref(app);

    delete client;
    agent_thread.join();
    close(client_socket);

    return status;
}
