// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <deque>
#include <sstream>

#include "../framework/logger.h"
#include "../framework/taskconfig.h"
#include "../framework/synchronousbridge.h"

class CliClient : public Logger, public SynchronousClient {
public:
    CliClient(const TaskConfig &config);

    void initialMsgToAgent(std::deque<std::string> &return_msgs) override;

    // msg is a new message from the agent.
    // push any return messages onto return_msgs.
    virtual void newEventFromAgent(std::deque<std::string> &return_msgs,
                                   const std::string &msg) override;
    void setHeader(const std::string &hdr) {
        in_progress_task = true;
        *out << hdr << std::flush;
        current_header = hdr;
    }
private:
    // To store the measurement details:
    struct {
        double latency, download, upload;
        std::string ticket, measurement_server, rating, isp, msg;
        int tls;
    } report = { -1.0, -1.0, -1.0, "no_support_ID", "", "", "", "", 0 };

    // Stuff to handle output:
    void show_message(const std::string &msg,  bool linefeed = true);
    void do_output(const char *msg, bool final = false);
    void do_output(double value, const char *msg, bool final = false);
    std::ostream *out;
    bool out_is_tty, out_quiet;
    std::string limiter = " ";
    std::ostringstream current_line;
    std::string current_header;
    // Set to true during upload and download:
    bool in_progress_task = false;
    // It latency result arrives asynchronously during
    // upload or download, we must wait until the end to show it:
    bool deferred_latency = false;

    const TaskConfig the_config;
};
