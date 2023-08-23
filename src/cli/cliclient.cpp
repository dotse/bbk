// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#include <iso646.h>
#include <stdio.h>
#include <io.h>
#else
#include <unistd.h>
#endif

#include <exception>
#include <iostream>
#include <iomanip>
#include <fstream>

#include "../json11/json11.hpp"
#include "cliclient.h"

CliClient::CliClient(const TaskConfig &config) :
    Logger("CLI"),
    out(&std::cout),
    the_config(config) {

    //setlocale(LC_ALL, "");

#ifdef _WIN32
    out_is_tty = _isatty(_fileno(stdout));
#else
    out_is_tty = isatty(fileno(stdout));
#endif
    limiter = the_config.value("limiter");
    out_quiet = (the_config.value("quiet") == "1" ||
                 the_config.value("logfile") == "-");
    if (!the_config.value("out").empty()) {
        out = new std::ofstream(the_config.value("out"), std::ofstream::app);
        out_is_tty = false;
    }
    report.measurement_server = the_config.value("server");
    if (the_config.value("pingsweep") != "1" &&
        the_config.value("list_measurements").empty()) {
        if (out_quiet) {
            // Block all output:
            out->clear(std::istream::eofbit);
        } else {
            *out << "Start: ";
            sayTime(*out);
            *out << ((the_config.value("mtype") == "ipv6") ? "  [ipv6]\n" : "\n");
        }
    }

    /*
    try {
        current_line.imbue(std::locale(""));
    } catch (std::exception &e) {
        log() << "cannot set locale: " << e.what();
    }
    */

    if (the_config.value("logfile") == "-")
        out_is_tty = false; // Otherwise output might be garbled by the log.
}

void CliClient::initialMsgToAgent(std::deque<std::string> &return_msgs) {
    return_msgs.push_back("{\"method\": \"clientReady\", \"args\": {}}");
}

void CliClient::newEventFromAgent(std::deque<std::string> &return_msgs,
                                  const std::string &msg) {
    std::string jsonerr;
    auto obj = json11::Json::parse(msg, jsonerr);
    if (!jsonerr.empty()) {
        if (BridgeTask::isAgentTerminatedMessage(msg))
            std::cerr << msg << std::endl;
        else
            err_log() << "JSON error: got " << msg;
        return_msgs.push_back(BridgeTask::msgToAgent("terminate"));
        return;
    }

    std::string event = obj["event"].string_value();
    auto arg_obj = obj["args"];

    if (event == "setInfo" && !arg_obj["logText"].string_value().empty()) {
        log() << "EVENT: setInfo logText";
    } else
        log() << "EVENT: " << event << ' ' << msg;

    if (event == "configuration") {
        if (!arg_obj["require_consent"].string_value().empty()) {
            // We need user consent to process personal data.
            // First fetch the legalese:
            json11::Json args = json11::Json::object {
                { "lang", "en" },
                { "format", "text" },
                { "consent", arg_obj["require_consent"] },
            };
            return_msgs.push_back(BridgeTask::msgToAgent("getContent", args.dump()));
            // When the content arrives, we'll show it to the user and
            // ask for consent.
            return;
        }
        report.isp = arg_obj["ispname"].string_value();
        if (!report.isp.empty())
            *out << "Network operator: " << report.isp << std::endl;

        if (the_config.value("ssl") == "1")
            report.tls = 1;
        int server_port = std::stoi(the_config.value("port"));
        if (report.measurement_server.empty()) {
            std::vector<json11::Json> vec = arg_obj["servers"].array_items();
            for (auto srv : vec) {
                if (srv["type"].string_value() == the_config.value("mtype")) {
                    std::string hostname = srv["url"].string_value();
                    auto pos = hostname.find(':');
                    if (pos != std::string::npos) {
                        server_port = std::stoi(hostname.substr(pos+1));
                        hostname.resize(pos);
                    }
                    if (report.tls) {
                        if (int tlsport = srv["tlsport"].int_value()) {
                            report.measurement_server = hostname;
                            server_port = tlsport;
                            break;
                        }
                    } else {
                        report.measurement_server = hostname;
                        break;
                    }
                }
            }
            if (report.measurement_server.empty()) {
                show_message("Error: no measurement server");
                return_msgs.push_back(BridgeTask::msgToAgent("terminate"));
                return;
            }
        }
        //*out << "Server: " << report.measurement_server << std::endl;
        std::string key = arg_obj["hashkey"].string_value();
        json11::Json out_args = json11::Json::object {
            { "serverUrl", report.measurement_server },
            { "serverPort", server_port },
            { "userKey", key },
            { "tls", report.tls },
        };
        if (the_config.value("pingsweep") == "1")
            return_msgs.push_back(BridgeTask::msgToAgent("pingSweep"));
        else
            return_msgs.push_back(BridgeTask::msgToAgent("startTest", out_args.dump()));
    } else if (event == "taskProgress") {
        std::string tst = arg_obj["task"].string_value();
        if (tst == "download" || tst == "upload" || tst == "uploadinfo") {
            double val = arg_obj["result"].number_value();
            do_output(val, " Mbit/s", false);
        }
    } else if (event == "taskStart") {
        std::string tst = arg_obj["task"].string_value();
        if (tst == "download") {
            setHeader("Download: ");
        } else if (tst == "upload") {
            setHeader("Upload:   ");
        }
    } else if (event == "taskComplete") {
        std::string tst = arg_obj["task"].string_value();
        if (tst == "global") {
            if (out_quiet) {
 
                // Enable output:
                out->clear(std::istream::goodbit);

                if (the_config.value("quiet") != "1")
                    *out << "\n\nRESULT: ";

                std::string measurement_id = arg_obj["MeasurementID"].string_value();
                *out << measurement_id << limiter 
                     << report.download << limiter
                     << report.upload << limiter
                     << report.latency << limiter
                     << report.measurement_server << limiter
                     << report.isp << limiter
                     << report.ticket;
                if (report.rating.empty())
                    *out << std::endl;
                else
                    *out << limiter << report.rating << std::endl;
            }
            return_msgs.push_back(BridgeTask::msgToAgent("quit"));
        } else if (tst == "latency") {
            auto res = arg_obj["result"].number_value();
            report.latency = res;
            if (in_progress_task) {
                deferred_latency = true;
            } else {
                setHeader("Latency:  ");
                do_output(report.latency, " ms", true);
            }
        } else if (tst == "download") {
            in_progress_task = false;
            auto res = arg_obj["result"].number_value();
            report.download = res;
            do_output(res, " Mbit/s", true);
        } else if (tst == "upload") {
            in_progress_task = false;
            auto res = arg_obj["result"].number_value();
            report.upload = res;
            do_output(res, " Mbit/s", true);
            if (deferred_latency) {
                *out << "Latency:  " << std::flush;
                do_output(report.latency, " ms", true);
            }
            // Since Measure.AutoSaveReport is false, we tell the agent we're done.
            // Any information can be sent in the args object.
            return_msgs.push_back(BridgeTask::msgToAgent("saveReport", "{}"));
        }
    } else if (event == "agentReady") {
        if (!the_config.value("list_measurements").empty()) {
            std::map<std::string, std::string> pars;
            pars["max"] = the_config.value("list_measurements");
            if (!the_config.value("list_from").empty())
                pars["from"] = the_config.value("list_from");
            // pars["key"] = ...
            json11::Json args = json11::Json(pars);
            return_msgs.push_back(BridgeTask::msgToAgent("listMeasurements",
                                                 args.dump()));
        } else {
            return_msgs.push_back(BridgeTask::msgToAgent("getConfiguration"));
        }
    } else if (event == "measurementList") {
        std::vector<json11::Json> vec = arg_obj["measurements"].array_items();
        if (out_quiet) {
            *out << arg_obj.string_value() << std::endl;
        } else if (vec.empty()) {
            *out << "No measurements found." << std::endl;
        } else {
            // {"id":83310,"down":113.994,"up":58.6238,"latency":6.28596,"server":"Stockholm","isp":"Stiftelsen for Internetinfrastruktur","ts":1519736433}
            *out << "ID\tDownload\tUpload\tLatency\tServer\tISP\tDate\n";
            for (auto &m : vec) {
                time_t ts = static_cast<time_t>(m["ts"].number_value());
                unsigned long id = static_cast<unsigned long>(m["id"].number_value());
                if (id && ts)
                    *out << id << "\t"
                         << m["down"].number_value() << "\t"
                         << m["up"].number_value() << "\t"
                         << m["latency"].number_value() << "\t"
                         << m["server"].string_value() << "\t"
                         << m["isp"].string_value() << "\t"
                         << dateString(ts) << "\n";
            }
            long n = static_cast<long>(arg_obj["remaining"].number_value());
            if (n > 0)
                *out << n << " older measurements\n";
        }
        return_msgs.push_back(BridgeTask::msgToAgent("terminate"));
    } else if (event == "report") {
        auto res = arg_obj["subscription"];
        if (res["status"].int_value() != 1)
            return;
        std::string isp = res["ispOperator"].string_value();
        if (!isp.empty() && isp != report.isp)
            *out << "Service provider: " << isp << std::endl;
        report.msg = res["ispInfoMessage"].string_value();
        if (!report.msg.empty())
            *out << "Message from service provider: " << report.msg << std::endl;
        std::string subs = res["ispSpeedName"].string_value();
        if (subs.empty())
            return;
        *out << "Subscription: " << subs << std::endl;
        auto sinfo = arg_obj["subscription_info"];
        for (auto &p1 : sinfo.array_items()) {
            for (auto &p2 : p1["categories"].array_items()) {
                if (p2["description"].string_value() == subs) {
                    try {
                        std::string good = p2["good"].string_value(),
                            acceptable = p2["acceptable"].string_value();
                        if (good.empty() || acceptable.empty())
                            return;
                        if (report.download*1000 >= std::stod(good))
                            report.rating = "GOOD";
                        else if (report.download*1000 >= std::stod(acceptable))
                            report.rating = "ACCEPTABLE";
                        else
                            report.rating = "BAD";
                        *out << "The download result is "
                             << report.rating << std::endl;
                        break;
                    } catch (...) {
                    }
                }
            }
        }
        if (report.rating == "BAD") {
            std::string bmsg = res["ispBadInfoMessage"].string_value();
            if (!bmsg.empty() && bmsg != report.msg)
                *out << "Message from service provider regarding the download "
                     << "result: " << bmsg << std::endl;
        }
    } else if (event == "measurementInfo") {
        std::string id = arg_obj["MeasurementID"].string_value();
        if (!id.empty())
            *out << "Measurement ID: " << id << std::endl;
        std::string imsg = arg_obj["ispInfoMessage"].string_value();
        if (!imsg.empty() && imsg != report.msg)
            *out << "Message from service provider: " << imsg << std::endl;
    } else if (event == "setInfo") {
        for (auto &p : arg_obj.object_items()) {
            std::string attr = p.first;
            std::string value = p.second.string_value();
            if (attr == "error") {
                if (!value.empty()) {
                    *out << "fatal error: " << value << std::endl;
                    break;
                }
            } else if (attr == "ticket") {
                report.ticket = value;
                *out << "Support ID: " << value << std::endl;
            } else if (attr == "contents") {
                json11::Json cobj = p.second;
                if (!cobj["consent"].string_value().empty() &&
                    !cobj["body"].string_value().empty()) {
                    show_message(cobj["body"].string_value());
                    show_message("Type Y to accept, N to decline: ", false);
                    std::string reply;
                    std::getline(std::cin, reply);
                    if (reply.find_first_of("Yy") != std::string::npos) {
                        return_msgs.push_back(BridgeTask::msgToAgent("getConfiguration",
                                                                    cobj.dump()));
                    } else {
                        return_msgs.push_back(BridgeTask::msgToAgent("terminate"));
                    }
                } else {
                    show_message("Server error.");
                    return_msgs.push_back(BridgeTask::msgToAgent("terminate"));
                }
            } else if (attr == "logText") {
            } else if (attr == "approxLatency") {
                show_message("Response time: " + value);
            } else if (attr == "bestServer") {
                if (value.empty())
                    show_message("error: no server available");
                else
                    show_message("Closest server: " + value);
                return_msgs.push_back(BridgeTask::msgToAgent("terminate"));
            } else if (attr == "msgToUser") {
                show_message(value);
            }
        }
    }
}

void CliClient::show_message(const std::string &msg, bool linefeed) {
    if (out_quiet)
        out->clear(std::istream::goodbit);
    *out << msg;
    if (linefeed)
        *out << std::endl;
    else
        *out << std::flush;
    if (out_quiet)
        out->clear(std::istream::eofbit);
}

void CliClient::do_output(double value, const char *msg, bool final) {
    if (out_is_tty) // Erase current line:
        *out << '\r';
    if (out_is_tty || final) {
        auto to_delete = current_line.str().size();
        current_line.str("");
        current_line << current_header << std::setw(10) << std::setprecision(3)
                     << std::fixed << value << msg;
        *out << current_line.str();
        if (to_delete > current_line.str().size())
            *out << std::string(to_delete - current_line.str().size(), ' ');
        *out << std::flush;
    }
    if (final) {
        current_line.str("");
        if (value <= 0)
            *out << " test failed";
        *out << std::endl;
    }
}

void CliClient::do_output(const char *msg, bool final) {
    if (out_is_tty) // Erase current line:
        *out << '\r';
    if (out_is_tty || final) {
        auto to_delete = current_line.str().size();
        current_line.str("");
        current_line << msg;
        *out << current_line.str();
        if (to_delete > current_line.str().size())
            *out << std::string(to_delete - current_line.str().size(), ' ');
        *out << std::flush;
    }
    if (final) {
        current_line.str("");
        *out << std::endl;
    }
}
