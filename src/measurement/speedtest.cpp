// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#include <iomanip>

#include "../http/sha1.h"

#include "speedtest.h"

#include "measurementagent.h"
#include "rpingtask.h"
// Alternative latency measurement:
#include "latencytask.h"
#include "warmuptask.h"
#include "tickettask.h"
#include "infotask.h"
#include "downloadtask.h"
// Alternative download measurement:
#include "wsdownloadtask.h"
#include "uploadtask.h"
// Alternative upload measurement: #include "wsuploadtask.h"
// Alternative to some parts of infotask:
#include "uploadinfotask.h"

SpeedTest::SpeedTest(MeasurementAgent *agent, const HttpHost &mserver,
                     const std::map<std::string, std::string> &report_data) :
    Task("SpeedTest"),
    the_agent(agent),
    mserv(mserver),
    report(report_data)
{
    bytesSentAtStart = SocketConnection::totBytesSent();
    bytesRecAtStart = SocketConnection::totBytesReceived();
    killChildTaskWhenFinished();
    upload_duration = std::stod(report["ulength"]);
    download_duration = std::stod(report["dlength"]);
    try {
        auto p = report.find("speedlimit");
        if (p != report.end()) {
            log() << "speedlimit " << p->second;
            speed_limit = std::stod(p->second);
            if (speed_limit < 0.5)
                speed_limit = 0.5;
            if (speed_limit <= 10.0) {
                initial_no_dconn = 2;
                max_no_dconn = 10;
                initial_no_uconn = 2;
                max_no_uconn = 4;
            }
        } else {
            log() << "no speedlimit";
        }
    } catch (...) {
        the_agent->sendToClient("setInfo", MeasurementTask::
                                json_obj("warning", "cannot parse SpeedLimit value"));
    }

}

double SpeedTest::start() {
    startObserving(the_agent);
    addNewTask(new TicketTask(mserv, report["key"], report["host"]), this);
    return 0.0;
}

void SpeedTest::taskMessage(Task *task) {
    if (!the_agent)
        return;

    std::string name = task->label(),
        message = task->message();

    if (task == info_task) {
        log() << "GOT INFO: " << message;
        if (message == "server upload timeout") {
            uploadComplete();
            return;
        }
        std::string err;
        auto obj = json11::Json::parse(message, err);
        if (!err.empty()) {
            err_log() << "JSON error, ignoring message";
            return;
        }
        std::string event = obj["event"].string_value();
        const json11::Json &args = obj["args"];
        // {"event": "measurementInfo",
        //    "args": {
        //       "ispname":"Telia","date":"Thu, 30 Nov 2017 15:25:31 +0000",
        //        "rating":"GOOD","MeasurementID":"109"
        //    }
        // }
        if (event == "measurementInfo") {
            the_agent->sendToClient("measurementInfo", args.dump());
            setResult("");
        } else if (event == "uploadInfo") {
            /* {
                   "event": "uploadInfo",
                   "args": {
                       "duration": 1.506616427,
                       "speed": 37348.559901815221
                   }
               }
            */
            double speed = args["speed"].number_value();
            double duration = args["duration"].number_value();

            log() << "uploadInfo Duration: " << duration
                  << " Speed: " << speed;
            if (duration >= upload_duration) {
                server_upload_speed = MeasurementTask::fValue(speed);
                uploadComplete();
            } else if (duration > 0)
                the_agent->sendTaskProgress("upload", speed,
                                            duration/upload_duration);
        } else {
            log() << "unknown event: " << event;
        }
    } else if (ProgressTask *ptask = dynamic_cast<ProgressTask *>(task)) {
        double speed = ptask->currentMbps();
        double progress = ptask->currentProgress();
        if (name != "upload")
            the_agent->sendTaskProgress(name, speed, progress);
        log() << name << " speed " << speed << " Mbit/s, progress " << progress;
    } else {
        log() << name << " message: " << message;
    }
}

void SpeedTest::taskFinished(Task *task) {
    std::string name = task->label(),
        result = task->result();

    if (task->wasKilled())
        log() << "Task " << name << " killed";
    else
        log() << "Task " << name << " finished, result: " << result;

    if (task == the_agent) {
        the_agent = nullptr;
        log() << "Agent gone, will exit";
        setResult("");
    } else if (task == info_task) {
        info_task = nullptr;
        if (report_sent_to_server)
            setResult("");
        return;
    }

    if (!the_agent)
        return;

    if (name == "ticket") {
        if (result.empty()) {
            json11::Json obj = json11::Json::object {
                { "error", "no network access to server" },
                { "errno", "C01" }
            };
            the_agent->sendToClient("setInfo", obj.dump());
            setResult("");
        } else {
            tstr = result;
            if (TicketTask *ttask = dynamic_cast<TicketTask *>(task))
                report["localip"] = ttask->localIp();

            the_agent->sendToClient("setInfo", MeasurementTask::
                                    json_obj("ticket", tstr));
            the_agent->sendToClient("taskStart", MeasurementTask::
                                    json_obj("task", "latency"));
            info_task = new InfoTask("measurement info", tstr,
                                     report["key"], mserv);

            addNewTask(info_task, this);
            RpingTask *t = new RpingTask("rping", tstr, mserv);
            addNewTask(t, this);

            // Send the ticket to the agent. The agent may use it to update
            // subscription into or fetch logs after the measurement is done.
            executeHandler(the_agent, tstr);
        }
    } else if (name == "measurementStart") {
        std::string err;
        auto obj = json11::Json::parse(result, err);
        if (!result.empty() && err.empty()) {
            std::string ip = obj["ip"].string_value();
            if (!ip.empty())
                log() << "Client external ip number: " << ip;
            if (local_latency) {
                double latency = obj["latency"].number_value();
                if (latency > 0) {
                    local_latency = false;
                    report["latency"] = std::to_string(latency);
                    log() << "Adjusted latency: " << latency;
                }
            }
        }
    } else if (name == "rping" || name == "httplatency") {
        if (name == "rping") {
            if (result.size()) {
                websocket_works = true;
            } else {
                addNewTask(new LatencyTask(tstr, mserv), this);
                return;
            }
        } else {
            local_latency = true;
            if (!result.size())
                result = "-1";
        }
        report["latency"] = result;
        the_agent->sendTaskComplete("latency", result);
        std::string url = "/measurementStarted?t=" + tstr;
        //url += "&uptick=120";

        HttpClientConnection::addUrlPars(url, report);
        log() << "measurementStart: " << mserv.hostname << url;
        addNewTask(new SingleRequestTask(url, "measurementStart",
                                                tstr, mserv), this);
        addNewTask(new WarmUpTask(tstr, mserv, initial_no_dconn, 3.0),
                          this);
    } else if (name == "download") {
        if (!result.size())
            result = "-1";
        the_agent->sendTaskComplete("download", result);
        report["download"] = result;
        the_agent->sendToClient("taskStart", MeasurementTask::
                                json_obj("task", "upload"));
        // UploadInfoTask fetches upload speed/progress from the server.
        // If the final speed sill hasn't arrived 3.0 seconds after the end of
        // the upload measurement, we give up and use the local upload speed
        // estimate which is lower since we can only count confirmed data.
        if (!websocket_works)
            addNewTask(new UploadInfoTask(tstr, mserv, upload_duration,
                                          upload_duration+3.0), this);
        UploadTask *t = new UploadTask(tstr, mserv, initial_no_uconn,
                                       max_no_uconn, upload_duration);
        if (speed_limit > 0)
            t->set_speedlimit(speed_limit);
        addNewTask(t, this);
    } else if (name == "saveReport") {
        uint64_t totBytesSent = SocketConnection::totBytesSent() - bytesSentAtStart;
        uint64_t totBytesReceived = SocketConnection::totBytesReceived() - bytesRecAtStart;

        json11::Json bytesInfo = json11::Json::object {
            { "totBytesReceived", std::to_string(totBytesReceived)},
            { "totBytesSent", std::to_string(totBytesSent) }
        };

        the_agent->sendToClient("setInfo", bytesInfo.dump());

        std::string err;
        auto obj = json11::Json::parse(result, err);
        if (!result.empty() && err.empty()) {
            the_agent->sendToClient("report", result);
            if (report["host"] == "none") {
                // Local measurement, no more info will arrive.
                setResult("");
                return;
            }
            log() << "INFO: " << result;
            // Wait at most 3 seconds for final info from server.
            if (info_task) {
                info_task->setInfoDeadline(3.0);
                info_task->resetTimer(0.1);
            } else {
                std::string url = "/getUpdate?t=" + tstr +
                    "&key=" + report["key"];
                addNewTask(new SingleRequestTask(url, "getUpdate", tstr,
                                                 mserv), this);
            }
        } else {
            the_agent->sendToClient("report", "{}");
            log() << "invalid saveReport: " << result;
            setResult("");
        }
    } else if (name == "warmup") {
        the_agent->sendToClient("taskStart", MeasurementTask::
                                json_obj("task", "download"));
        //WsDownloadTask *t = new WsDownloadTask(tstr, mserv, initial_no_dconn,
        DownloadTask *t = new DownloadTask(tstr, mserv, initial_no_dconn,
                                           max_no_dconn, download_duration);
        log() << "t->set_speedlimit " << speed_limit;
        if (speed_limit > 0)
            t->set_speedlimit(speed_limit);
        addNewTask(t, this);
    } else if (name == "upload") {
        local_upload_speed = result.size() ? result : "0";
        if (server_upload_speed.empty()) {
            if (info_task) {
                // Wait for up to 3 seconds for server upload speed:
                info_task->setUploadDeadline(3.0);
                info_task->resetTimer(0.1);
            } else {
                // Pointless to wait for server upload info.
                uploadComplete();
            }
        }
    } else if (name == "getUpdate") {
        std::string err;
        auto obj = json11::Json::parse(result, err);
        std::string event = obj["event"].string_value();
        if (err.empty() && event == "measurementInfo") {
            const json11::Json &args = obj["args"];
            the_agent->sendToClient("measurementInfo", args.dump());

        } else {
            err_log() << "JSON error, ignoring message";
            the_agent->sendToClient("measurementInfo", "{}");
        }
        setResult("");
    } else {
        log() << "unknown task, ignoring";
    }
}

void SpeedTest::uploadComplete() {
    if (report.find("upload") != report.end()) {
        log() << "Upload already set: " << report["upload"];
        return;
    }

    // Local and server speed calculations are complete.
    // Use local calculation only if server calculation failed:
    std::string result = server_upload_speed;
    if (result.empty() || (result == "0" && !local_upload_speed.empty()))
        result = local_upload_speed;
    report["upload"] = result;
    the_agent->sendTaskComplete("upload", result);
    auto p = report.find("autosave");
    if (p == report.end() || p->second != "false")
        doSaveReport();
    else
        log() << "Wait for client to be ready for saving report";
    if (info_task)
        info_task->setUploadDeadline(-1.0);
}

void SpeedTest::addToReport(const std::string &attr, const std::string &val) {
    report[attr] = val;
}

void SpeedTest::doSaveReport(const json11::Json &args) {
    if (report_sent_to_server)
        return;
    report_sent_to_server = true;
    std::string url("/saveReport?t=" + tstr);

    for (auto p : args.object_items()) {
        const std::string &attr = p.first;
        if (report.find(attr) == report.end())
            report[attr] = p.second.string_value();
    }
    HttpClientConnection::addUrlPars(url, report);

    log() << "saveReport " << url;
    addNewTask(new SingleRequestTask(url, "saveReport",
                                     tstr, mserv), this);
}
