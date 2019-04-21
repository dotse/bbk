#include <QApplication>
#include <QWebEngineView>

#include <fstream>
#include <thread>
#include <chrono>

#include "../measurement/measurementagent.h"
#include "../cli/utils.h"
#include "../http/websocketbridge.h"

namespace {

    void runAgent(BridgeTask *bridge, const TaskConfig &cfg) {
        CookieFile cf(cfg.value("config_file"));
        HttpHost webserver(cfg.value("Measure.Webserver"), 80, "", 0, &cf);
        EventLoop loop;
        bridge->setAgent(new MeasurementAgent(cfg, webserver));
        loop.addTask(bridge);
        loop.runUntilComplete();
    }
}

int main(int argc, char *argv[])
{
    TaskConfig agent_cfg, config;
    if (!parseArgs(argc, argv, config, agent_cfg))
        return 1;
    agent_cfg.set("Measure.AutoSaveReport", "true");
    std::ofstream log_file;
    if (config.value("logfile") != "-") {
        log_file.open(config.value("logfile"));
        Logger::setLogFile(log_file);
    }

    config.set("listen_pw", MeasurementAgent::createHashKey(12));
    config.set("listen", "0");
    config.set("listen_addr", "127.0.0.1");
    config.set("browser",  "2");
    agent_cfg.set("config_file", config.value("config_file"));

    WebsocketBridge *bridge = new WebsocketBridge(nullptr, config);
    std::thread agent_thread(runAgent, bridge, agent_cfg);

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    QWebEngineView view;
    view.resize(700, 950);
    view.show();

    // Wait for a few seconds for the agent to start listening on a port
    for (unsigned int i = 0; bridge->url().empty() && i < 400; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(1+i/10));

    int ret;
    if (bridge->url().empty()) {
        std::cerr << "cannot start measurement engine";
        ret = 1;
    } else {
        view.setUrl(QUrl(bridge->url().c_str()));
        ret = app.exec();
    }

    bridge->die();
    agent_thread.join();
    return ret;
}
