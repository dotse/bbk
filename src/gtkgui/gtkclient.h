// Copyright (c) 2019 Internetstiftelsen
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <gtk/gtk.h>

#include "../json11/json11.hpp"
#include "../framework/unixdomainclient.h"
#include "../framework/bridgetask.h"
#include "../framework/logger.h"

class GtkClient : public Logger {
public:
    GtkClient(const TaskConfig &config, int unix_domain_peer);
    ~GtkClient();

    static void activate(GtkApplication *app, gpointer user_data);
    static gboolean poll_agent(gint fd, GIOCondition c, gpointer data);
    static void start_measurement(GtkWidget *widget, gpointer data);
    static void update_serverlist(GtkWidget *widget, gpointer data);

    void doActivate(GtkApplication *app);
    void doStartMeasurement();

    GtkWidget *staticLabel(const char *label, GtkAlign align = GTK_ALIGN_END);
    GtkWidget *dynamicLabel(GtkAlign align = GTK_ALIGN_START);
    void setLabel(GtkWidget *widget, const std::string &label);
    void pushToAgent(const std::string &method, const std::string &arg = "{}") {
        if (arg.size() > 2)
            log() << "Send: " << method << " " << arg;
        else
            log() << "Send: " << method;
        ud_client.pushToAgent(BridgeTask::msgToAgent(method, arg));
    }

    std::string myStrFormat(double x);
    void gotSettings(const json11::Json &obj);
    void gotInfo(const json11::Json &obj);
    void gotReport(const json11::Json &obj);
    void gotTaskComplete(const json11::Json &obj);
    void newEventFromAgent(const std::string &msg);
    std::string pollAgent() {
        return ud_client.pollAgent();
    }
private:
    enum class MState {
        IDLE, RESTARTING, MEASURING, FINISHED, ERROR
    };
    void setState(MState newState);
    void updateServerBox();
    void reset();
    MState state = MState::IDLE;
    bool got_ticket = false;
    bool user_abort = false;
    GtkWidget *main_window;
    GtkWidget *start_button;
    GtkWidget *label_isp, *label_ticket, *label_date,
        *label_ip, *label_pip, *label_id,
        *label_download, *label_upload, *label_latency,
        *label_ipsmsg, *label_evaluation, *label_message;
    GtkWidget *server_box, *iptype_box;
    int peer_fd;
    UnixDomainClient ud_client;
    json11::Json settings;
    const TaskConfig the_config;
};
