// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <string>
#include <map>
#include <sstream>

#include "../framework/bridgetask.h"
#include "../json11/json11.hpp"
#include "../http/httphost.h"
#include "../http/cookiefile.h"

class SpeedTest;

class MeasurementAgent : public Task {
public:
    MeasurementAgent(const TaskConfig &config, const HttpHost &webserver);
    void taskMessage(Task *task) override;
    void taskFinished(Task *task) override;

    void handleExecution(Task *sender, const std::string &msg) override;
    void sendToClient(const std::string &method,
                      const std::string &jsonobj = "{}") {
        if (bridge)
            bridge->sendMsgToClient(method, jsonobj);
    }
    void sendTaskComplete(const std::string &t, const std::string &res = "");
    void sendTaskProgress(const std::string &taskname,
                          double speed, double progress);
    void accumulateLog() {
        setLogFile(accumulated_log);
    }
    void appendLog(const std::string &str) {
        accumulated_log << "\nAppend " << str.size() << "\n" << str;
    }
    void sendLogToServer();
private:
    std::string getDefaultConfig();
    bool isValidHashkey(const std::string &key);
    void pollBridge(const std::string &msg);
    static bool isValidJson(const std::string &s) {
        std::string err;
        auto obj = json11::Json::parse(s, err);
        return err.empty();
    }

    void handleMsgFromClient(const std::string &method,
                             const json11::Json &args);
    void handleConfigurationOption(const std::string &name,
                                   const std::string &value);
    void uploadComplete();
    void doSaveReport();
    void resetCurrentTest();
    BridgeTask *bridge = nullptr;
    SpeedTest *current_test = nullptr;
    std::string current_ticket;
    std::ostringstream accumulated_log;

    // Initial state is IDLE. When client says "startTest", state becomes
    // STARTED. When test is done, we send "testComplete global" to client
    // and set state to FINISHED. When client sends resetTest, state will
    // be reset to IDLE.
    // If client sends abortTest in state STARTED, state becomes ABORTED.
    enum class MeasurementState { IDLE, STARTED, FINISHED, ABORTED };
    MeasurementState state = MeasurementState::IDLE;

    // If the client doesn't manage keys, we store them here:
    CookieManager *key_store;

    std::string force_key;

    // The web server and the measurement server:
    HttpHost wserv, mserv;

    // Default value, might be modified by the client
    std::string wserv_contentsurl = "/api/content";
    std::string wserv_measurementsurl = "/api/measurements";
    std::string wserv_settingsurl = "/api/servers";
    std::string settings_result;

    // Info to be included each time measurement result is sent:
    std::map<std::string, std::string> report_template;

    TaskConfig cfgOptions;
    std::string options_filename;
};
