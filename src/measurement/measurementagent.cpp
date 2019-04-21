// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#include <clocale>
#include <sstream>
#include <iomanip>

#include "speedtest.h"

#include "defs.h"
#include "singlerequesttask.h"
#include "pingsweeptask.h"
#include "measurementagent.h"

MeasurementAgent::MeasurementAgent(const TaskConfig &config,
                                   const HttpHost &webserver) :
    Task("MeasurementAgent"),
    wserv(webserver) {

    killChildTaskWhenFinished();

    key_store = wserv.cmgr ? wserv.cmgr : new CookieManager();

    // Default values; could be changed by the client app (gui):
    report_template["appname"] = measurement::appName;
    report_template["appver"] = measurement::appVersion;
    report_template["dlength"] = "10";
    report_template["ulength"] = "10";

    for (auto p : config.cfg())
        handleConfigurationOption(p.first, p.second);
}

void MeasurementAgent::taskMessage(Task *task) {
    // std::string name = task->label(), message = task->message();
    if (PingSweepTask *ptask = dynamic_cast<PingSweepTask *>(task)) {
        while (true) {
            std::string res = ptask->getResult();
            if (res.empty())
                break;
            sendToClient("setInfo", MeasurementTask::
                         json_obj("approxLatency", res));
        }
    }
}

void MeasurementAgent::taskFinished(Task *task) {
    std::string name = task->label(),
        result = task->result();

    if (task->wasKilled())
        log() << "Task " << name << " killed";
    else
        log() << "Task " << name << " finished, result: " << result;

    if (task == bridge) {
        bridge = nullptr;
        state = MeasurementState::ABORTED;
        setResult("");
        return;
    }

    if (task == current_test) {
        current_test = nullptr;
        state = MeasurementState::FINISHED;
        sendTaskComplete("global");
        return;
    }

    if (task->wasKilled() || !bridge || state == MeasurementState::ABORTED)
        return;

    if (name == "msettings") {
        // Check that the result is valid JSON:
        std::string err;
        auto obj = json11::Json::parse(result, err);
        if (result.empty() || !err.empty()) {
            settings_result = getDefaultConfig();
        } else {
            if (result[result.size()-1] == '}') {
                std::string newkey = obj["hashkey"].string_value();
                std::string hashkey = !force_key.empty() ? force_key :
                    key_store->getCookieVal("hash_key", wserv.hostname);
                if (newkey == hashkey) {
                    // Nothing changed.
                } else if (isValidHashkey(hashkey)) {
                    // We already had a key. Try to insert it into settings.
                    if (newkey.empty()) {
                        result.resize(result.size()-1);
                        ((result += ",\"hashkey\":\"") += hashkey) += "\"}";
                    } else {
                        auto pos = result.find('"' + newkey + '"');
                        if (pos != std::string::npos)
                            result.replace(pos+1, newkey.size(), hashkey);
                    }
                } else if (isValidHashkey(newkey)) {
                    // We had no valid key. Save the one we got.
                    std::string domain = wserv.hostname;
                    if (domain.size() >= 18 && domain.substr(domain.size() - 18)
                        == "bredbandskollen.se")
                        domain = ".bredbandskollen.se";
                    std::string line = "hash_key=" + newkey +
                        "; max-age=999999999; domain=" + domain;
                    key_store->setCookie(line, wserv.hostname, "/");
                    log() << "SET COOKIE " << line;
                }
            }
            settings_result = result;
        }
        sendToClient("configuration", settings_result);
        key_store->save();
    } else if (name == "contents") {
        sendToClient("setInfo", "{\"contents\": " +
                                result + "}");
    } else if (name == "pingsweep") {
        log() << "Best server: " << result;
        sendToClient("setInfo", "{\"bestServer\": \"" + result + "\"}");
    } else if (name == "list_measurements") {
        sendToClient("measurementList", result);
    } else if (name == "checkHost") {
        if (HttpClientTask *t = dynamic_cast<HttpClientTask *>(task)) {
            sendToClient("hostCheck", MeasurementTask::
                json_obj(t->serverHost(), result.empty() ? "" : "1"));
            log() << "checkHost: " << result;
        }
    } else if (name == "measurementStart") {
        std::string err;
        auto obj = json11::Json::parse(result, err);

        if (err.empty()) {
            sendToClient("setInfo", result);
            log() << "measurementStart: " << result;
        } else {
            sendToClient("setInfo", "{}");
            warn_log() << "invalid measurementStart: " << result;
        }
    } else if (name == "setSubscription") {
        log() << "setSubscription done";
    } else {
        log() << "unknown task, ignoring";
    }

}

void MeasurementAgent::pollBridge(const std::string &msg) {
    if (msg.empty())
        return;
    std::string err;
    auto obj = json11::Json::parse(msg, err);
    if (!err.empty()) {
        err_log() << "JSON error, ignoring message: " << msg;
        return;
    }
    std::string method = obj["method"].string_value();
    const json11::Json &args = obj["args"];
    handleMsgFromClient(method, args);
}

void MeasurementAgent::sendTaskComplete(const std::string &t,
                                        const std::string &res) {
    if (!bridge)
        return;
    std::string args = "{\"task\": \"" + t + "\"";
    if (!res.empty())
        args += ", \"result\": " + res;
    sendToClient("taskComplete", args + "}");
}

bool MeasurementAgent::isValidHashkey(const std::string &key) {
    if (key.size() >= 12 && key.size() <= 40 && std::string::npos ==
        key.find_first_not_of("0123456789ABCDEFabcdef-"))
        return true;
    return false;
}

std::string MeasurementAgent::getDefaultConfig() {
    // Either the client doesn't want settings to be fetched
    // (wserv.hostname is "none"), or we failed to fetch settings.
    // If the client has supplied a measurement server, or if the domain is
    // bredbandskollen.se, we can create a valid config.

    std::string mserver4 = mserv.hostname, mserver6 = mserv.hostname,
        domain = wserv.hostname;
    if (domain == "none") {
        domain = mserv.hostname;
    } else if (domain.size() >= 18 && domain.substr(domain.size() - 18)
               == "bredbandskollen.se" && mserver4.empty()) {
        // Perhaps a client dns failure.
        mserver4 = "192.36.30.2";
        mserver6 = "2001:67c:2ff4::2";
    }
    std::string hashkey = !force_key.empty() ? force_key :
        key_store->getCookieVal("hash_key", domain);
    if (hashkey.empty()) {
        hashkey = createHashKey();
        key_store->setCookie("hash_key="+hashkey+";max-age=999999999",
                             domain, "/");
    }

    if (mserver4.empty()) {
        json11::Json settings = json11::Json::object {
            { "ispname", "" },
            { "hashkey", hashkey }
        };
        return settings.dump();
    }

    json11::Json settings = json11::Json::object {
        { "servers", json11::Json::array {
                json11::Json::object {
                    { "url", mserver4 },
                    { "name", mserver4 },
                    { "type", "ipv4" }
                },
                json11::Json::object {
                    { "url", mserver6 },
                    { "name", mserver6 },
                    { "type", "ipv6" }
                }
            }
        },
        { "ispname", "" },
        { "hashkey", hashkey }
    };
    return settings.dump();
}

void MeasurementAgent::handleMsgFromClient(const std::string &method,
                                           const json11::Json &args) {
    log() << "handleMsgFromClient " << method;
    if (method == "clientReady") {
        sendToClient("agentReady");
    } else if (method == "getConfiguration") {
        dbg_log() << "Webserver: " << wserv.hostname;
        if (wserv.hostname == "none") {
            settings_result = getDefaultConfig();
            sendToClient("configuration", settings_result);
        } else {
            std::map<std::string, std::string> attrs;
            std::string hashkey = !force_key.empty() ? force_key :
                key_store->getCookieVal("hash_key", wserv.hostname);
#ifdef USE_GNUTLS
            if (mserv.is_tls)
                attrs["ssl"] = "1";
#endif
            if (isValidHashkey(hashkey))
                attrs["key"] = hashkey; // attrs["nokey"] = "";
            if (!args["consent"].string_value().empty())
                attrs["consent"] = args["consent"].string_value();
            std::string url = wserv_settingsurl;
            HttpClientConnection::addUrlPars(url, attrs);
            log() << wserv.hostname << ':' << wserv.port
                  << " " << url;
            addNewTask(new SingleRequestTask(url, "msettings",
                                             "", wserv), this);
        }
    } else if (method == "getContent") {
        std::map<std::string, std::string> attrs;
        for (auto p : args.object_items())
            if (!p.second.string_value().empty())
                attrs[p.first] = p.second.string_value();
        std::string url = wserv_contentsurl;
        HttpClientConnection::addUrlPars(url, attrs);
        addNewTask(new SingleRequestTask(url, "contents",
                                         "", wserv), this);
    } else if (method == "setConfigurationOption") {
        for (auto p : args.object_items())
            handleConfigurationOption(p.first, p.second.string_value());
    } else if (method == "saveReport") {
        if (current_test)
            current_test->doSaveReport(args);
    } else if (method == "checkHost") {
        std::string hostname = args["server"].string_value();
        HttpHost tmpServer(wserv);
        tmpServer.hostname = hostname;
        addNewTask(new SingleRequestTask("/pingsweep/check", "checkHost",
                                         "", tmpServer), this);
    } else if (method == "startTest") {
        log() << "Args: " << args.dump();
        if (state == MeasurementState::IDLE) {
            std::string mserver = args["serverUrl"].string_value();
            std::string key = args["userKey"].string_value();
#ifdef USE_GNUTLS
            mserv.is_tls = static_cast<bool>(args["tls"].int_value());
            mserv.port = mserv.is_tls ? 443 : 80;
#else
            mserv.port = 80;
#endif
            if (args.object_items().find("serverPort") !=
                args.object_items().end()) {
                int p = args["serverPort"].int_value();
                if (p && p > 0 && p < 65536) {
                    mserv.port = static_cast<uint16_t>(p);
                    log() << "measurement server port number " << p;
                } else
                    err_log() << "invalid port number, will use default";
            }
            if (mserver.empty() || !isValidHashkey(key)) {
                sendToClient("setInfo","{\"error\": "
                    "\"startTest requires parameters serverUrl and userKey\"}");
                state = MeasurementState::FINISHED;
                sendTaskComplete("global");
                return;
            }
            log() << "got measurement server " << mserver;
            mserv.hostname = mserver;
            report_template["host"] = wserv.hostname;
            report_template["key"] = key;
            current_test = new SpeedTest(this, mserv, report_template);
            state = MeasurementState::STARTED;
            addNewTask(current_test, this);
        } else {
            err_log() << "Must do resetTest before starting a new test";
        }
    } else if (method == "abortTest") {
        if (state == MeasurementState::STARTED) {
            if (current_test) {
                state = MeasurementState::ABORTED;
                abortTask(current_test);
            } else {
                state = MeasurementState::FINISHED;
            }
        } else {
            err_log("got abortTest when not in measurement");
        }
    } else if (method == "resetTest") {
        switch (state) {
        case MeasurementState::FINISHED:
            state = MeasurementState::IDLE;
            sendToClient("agentReady");
            mserv.hostname.clear();
            current_ticket.clear();
            break;
        case MeasurementState::IDLE:
            break;
        case MeasurementState::ABORTED:
        case MeasurementState::STARTED:
            err_log("got resetTest during measurement");
            sendToClient("setInfo",
                "{\"error\": \"got resetTest during measurement\"}");
            break;
        }
    } else if (method == "setSubscription") {
        if (current_ticket.empty()) {
            err_log() << "cannot set subscription: latest measurement missing";
            return;
        }
        std::string id = args["speedId"].string_value();
        if (id.empty()) {
            long n = args["speedId"].int_value();
            if (n <= 0) {
                err_log() << "cannot set subscription: no speedId";
                return;
            }
            id = std::to_string(n);
        }
        std::string url = "/setSubscription?t=" + current_ticket + "&key=" +
            report_template["key"] + "&speed=" + id;
        addNewTask(new SingleRequestTask(url, "setSubscription",
                                         current_ticket, mserv), this);
    } else if (method == "listMeasurements") {
        std::map<std::string, std::string> uargs;
        uargs["key"] = key_store->getCookieVal("hash_key", wserv.hostname);
        for (auto p : args.object_items())
            uargs[p.first] = p.second.string_value();
        if (isValidHashkey(uargs["key"])) {
            std::string url = wserv_measurementsurl;
            HttpClientConnection::addUrlPars(url, uargs);
            log() << "Get URL: " << url;
            addNewTask(new SingleRequestTask(url, "list_measurements",
                                             "lmtask", wserv), this);
        } else {
            sendToClient("measurementList", "{\"measurements\":[]}");
        }
    } else if (method == "pingSweep") {
        addNewTask(new PingSweepTask(settings_result, wserv), this);
    } else if (method == "terminate") {
        bridge = nullptr;
        setResult("");
    } else if (method == "quit") {
        bridge = nullptr;
        setResult("");
    } else {
        log() << "unknown command";
    }
}

void MeasurementAgent::handleExecution(Task *sender, const std::string &msg) {
    if (sender == bridge) {
        pollBridge(msg);
    } else if (sender == current_test) {
        current_ticket = msg;
        dbg_log() << "current ticket: " << current_ticket;
    } else if (!bridge) {
        if (BridgeTask *bt = dynamic_cast<BridgeTask *>(sender)) {
            log() << "Bridge connected";
            bridge = bt;
            pollBridge(msg);
        }
    }
}

void MeasurementAgent::sendTaskProgress(const std::string &taskname,
                                        double speed, double progress) {
    // We can't use locale dependent number formating in JSON!
    std::ostringstream json;
    json.imbue(std::locale("C"));
    json << "{\"task\": \"" << taskname
         << "\", \"result\": " << speed
         << ", \"progress\": " << progress << "}";
    sendToClient("taskProgress", json.str());
}

void MeasurementAgent::handleConfigurationOption(const std::string &name,
                                                 const std::string &value) {
    log() << "option: " << name << " value: " << value;
    if (name == "Logging.LogToConsole") {
        if (!value.empty()) {
            Logger::setLogFile(std::cerr);
        }
    } else if (name.substr(0, 7) == "Client.") {
        std::string attr = name.substr(7);
        if (!attr.empty() && attr.size() <= 20 && std::string::npos ==
            attr.find_first_not_of("abcdefghijklmnopqrstuvwxyz")) {
            // TODO: check not reserved!
            if (attr == "hashkey")
                force_key = value;
            else
                report_template[attr] = value;
        } else {
            log() << "will ignore option " << name;
        }
    } else if (name.substr(0, 7) == "Report.") {
        std::string attr = name.substr(7);
        if (!attr.empty() && attr.size() <= 20 && std::string::npos ==
            attr.find_first_not_of("abcdefghijklmnopqrstuvwxyz") &&
            current_test)
            // TODO: check not reserved!
            current_test->addToReport(attr, value);
        else
            log() << "will ignore option " << name;
    } else if (name == "Measure.IpType") {
        wserv.iptype = (value == "ipv6") ? 6 : 4;
        std::string s = MeasurementTask::getLocalAddress();
        if (!s.empty())
            HttpClientTask::setLocalAddress(s, wserv.iptype);
    } else if (name == "Measure.Webserver") {
        wserv.hostname = value;
    } else if (name == "Measure.Server") {
        mserv.hostname = value;
    } else if (name == "Measure.LocalAddress") {
        if (!HttpClientTask::setLocalAddress(value, wserv.iptype))
            setError("cannot use local address");
#ifdef USE_GNUTLS
    } else if (name == "Measure.TLS") {
        if (value == "1") {
            mserv.is_tls = true;
            wserv.is_tls = true;
            wserv.port = 443;
        } else {
            mserv.is_tls = false;
            wserv.is_tls = false;
            wserv.port = 80;
        }
#endif
    } else if (name == "Measure.ProxyServerUrl") {
        wserv.proxyHost = value;
        mserv.proxyHost = value;
    } else if (name == "Measure.ProxyServerPort") {
        try {
            wserv.proxyPort = static_cast<uint16_t>(stoi(value));
        } catch (...) {
            wserv.proxyPort = 80;
        }
        mserv.proxyPort = wserv.proxyPort;
    } else if (name == "Measure.LoadDuration") {
        double duration = 10.0;
        try {
            duration = stod(value);
        } catch (...) {
        }
        if (duration < 2.0)
            duration = 2.0;
        else if (duration > 10.0)
            duration = 10.0;
        report_template["dlength"] = std::to_string(duration);
        report_template["ulength"] = std::to_string(duration);
    } else if (name == "Measure.SpeedLimit") {
        report_template["speedlimit"] = value;
    } else if (name == "Measure.AutoSaveReport") {
        report_template["autosave"] = value;
    } else if (name == "Measure.SettingsUrl") {
        wserv_settingsurl = value;
    } else if (name == "Measure.ContentsUrl") {
        wserv_contentsurl = value;
    } else if (name == "Measure.MeasurementsUrl") {
        wserv_measurementsurl = value;
    } else {
        log() << name << ": unknown option";
    }
}
