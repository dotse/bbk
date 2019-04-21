// Copyright (c) 2019 Internetstiftelsen
// Written by Göran Andersson <initgoran@gmail.com>

#include <stdio.h>

#include "gtkclient.h"
#include <glib-unix.h>

void GtkClient::newEventFromAgent(const std::string &msg) {
    log() << "Got: " << msg;

    std::string jsonerr;
    auto obj = json11::Json::parse(msg, jsonerr);
    if (!jsonerr.empty()) {
        if (BridgeTask::isAgentTerminatedMessage(msg))
            setLabel(label_message, msg);
        else {
            setLabel(label_message, "JSON error: got " + msg);
            err_log() << "JSON error";
        }
        pushToAgent("terminate");
        return;
    }

    std::string event = obj["event"].string_value();
    auto arg_obj = obj["args"];

    while (state == MState::MEASURING) {
        if (event == "taskProgress") {
            std::string tst = arg_obj["task"].string_value();
            double val = arg_obj["result"].number_value();
            std::string p = myStrFormat(val) + " Mbit/s";
            if (tst == "download")
                setLabel(label_download, p);
            else if (tst == "upload" || tst == "uploadinfo")
                setLabel(label_upload, p);
        } else if (event == "taskStart") {
        } else if (event == "taskComplete") {
            gotTaskComplete(arg_obj);
        } else if (event == "report") {
            gotReport(arg_obj);
        } else if (event == "measurementInfo") {
            std::string id = arg_obj["MeasurementID"].string_value();
        } else {
            break;
        }
        return;
    }

    if (event == "configuration") {
        gotSettings(arg_obj);
    } else if (event == "agentReady") {
        if (state == MState::IDLE)
            pushToAgent("getConfiguration");
        else if (state == MState::RESTARTING)
            doStartMeasurement();
    } else if (event == "measurementList") {
    } else if (event == "setInfo") {
        gotInfo(arg_obj);
    }
}

void GtkClient::reset() {
    pushToAgent("resetTest");
    setLabel(label_message, "");
    got_ticket = false;
    user_abort = false;
    setLabel(label_ticket, "");
    setLabel(label_latency, "");
    setLabel(label_download, "");
    setLabel(label_upload, "");
    setLabel(label_evaluation, "");
}

std::string GtkClient::myStrFormat(double x) {
    char strBuf[80];
    snprintf(strBuf, sizeof(strBuf), "%7.2f", x);
    return std::string(strBuf);
}

void GtkClient::gotReport(const json11::Json &obj) {
    setLabel(label_pip, obj["localip"].string_value());
}

void GtkClient::gotTaskComplete(const json11::Json &obj) {
    std::string tst = obj["task"].string_value();
    auto res = obj["result"].number_value();
    if (tst == "global") {
        if (got_ticket || user_abort) {
            setState(MState::FINISHED);
        } else {
            setState(MState::ERROR);
            pushToAgent("resetTest");
        }
    } else if (tst == "latency")
        setLabel(label_latency, myStrFormat(res) + " ms");
    else if (tst == "download")
        setLabel(label_download, myStrFormat(res) + " Mbit/s");
    else if (tst == "upload")
        setLabel(label_upload, myStrFormat(res) + " Mbit/s");
}

void GtkClient::gotInfo(const json11::Json &obj) {
    for (auto &p : obj.object_items()) {
        std::string attr = p.first;
        std::string value = p.second.string_value();
        if (attr == "error") {
            if (!value.empty())
                setLabel(label_message, "Error: " + value);
        } else if (attr == "ticket") {
            setLabel(label_ticket, value);
            got_ticket = true;
        } else if (attr == "logText") {
        } else if (attr == "msgToUser") {
            setLabel(label_message, value);
        }
    }
}

void GtkClient::gotSettings(const json11::Json &obj) {
    if (state == MState::ERROR) {
        setState(MState::IDLE);
    }
    settings = obj;
    setLabel(label_date, dateString());
    setLabel(label_isp, settings["ispname"].string_value());
    setLabel(label_ip, settings["ip"].string_value());
    updateServerBox();
}

void GtkClient::setLabel(GtkWidget *widget, const std::string &label) {
    gtk_label_set_text(GTK_LABEL(widget), label.c_str());
}

void GtkClient::updateServerBox() {

    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(server_box));

    std::set<std::string> srvName;
    std::string mtype(gtk_combo_box_text_get_active_text
                      (GTK_COMBO_BOX_TEXT(iptype_box)));
    bool tls = false;
    for (auto srv : settings["servers"].array_items()) {
        if (srv["type"].string_value() != mtype)
            continue;
        if (tls && srv["tlsport"].int_value() == 0)
            continue;
        std::string name = srv["name"].string_value();
        if (srvName.find(name) != srvName.end())
            continue;
        srvName.insert(name);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(server_box),
                                       name.c_str());
    }

    if (srvName.empty())
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(server_box),
                                       "No Server");

    gtk_combo_box_set_active(GTK_COMBO_BOX(server_box), 0);
}

gboolean GtkClient::poll_agent(gint , GIOCondition c, gpointer data) {
    auto self = reinterpret_cast<GtkClient *>(data);
    if (c & G_IO_HUP) {
        return FALSE;
    }
    while (true) {
        auto msg = self->pollAgent();
        if (msg.empty())
            break;
        self->newEventFromAgent(msg);
    }
    return TRUE;
}

GtkClient::GtkClient(const TaskConfig &config, int unix_domain_peer) :
    Logger("GUI"),
    peer_fd(unix_domain_peer),
    ud_client(peer_fd),
    the_config(config) {
    pushToAgent("clientReady");
}

GtkClient::~GtkClient() {
    pushToAgent("terminate");
}

void GtkClient::start_measurement(GtkWidget *, gpointer data) {
    auto self = reinterpret_cast<GtkClient *>(data);
    self->doStartMeasurement();
}

void GtkClient::update_serverlist(GtkWidget *, gpointer data) {
    auto self = reinterpret_cast<GtkClient *>(data);
    self->updateServerBox();
}

void GtkClient::doStartMeasurement() {
    if (state == MState::MEASURING) {
        pushToAgent("abortTest");
        user_abort = true;
        return;
    } else if (state == MState::FINISHED) {
        setState(MState::RESTARTING);
        return;
    } else if (state == MState::ERROR) {
        pushToAgent("getConfiguration");
        return;
    }
    setLabel(label_date, dateString());

    setState(MState::MEASURING);
    std::string mtype(gtk_combo_box_text_get_active_text
                      (GTK_COMBO_BOX_TEXT(iptype_box)));
    bool tls = false;

    std::string server_name(gtk_combo_box_text_get_active_text
                            (GTK_COMBO_BOX_TEXT(server_box)));

    int server_port = 80;
    std::string hostname;
    std::set<std::string> srvName;
    for (auto srv : settings["servers"].array_items()) {
        if (srv["type"].string_value() != mtype)
            continue;
        if ( srv["name"].string_value() != server_name)
            continue;
        if (tls && srv["tlsport"].int_value() == 0)
            continue;
        hostname = srv["url"].string_value();
        auto pos = hostname.find(':');
        if (pos != std::string::npos) {
            server_port = std::stoi(hostname.substr(pos+1));
            hostname.resize(pos);
        }
        if (tls)
            server_port = srv["tlsport"].int_value();
        break;
    }

    std::string key = settings["hashkey"].string_value();
    json11::Json out_args = json11::Json::object {
        { "serverUrl", hostname },
        { "serverPort", server_port },
        { "userKey", key },
        { "tls", tls },
    };
    pushToAgent("startTest", out_args.dump());
}

void GtkClient::setState(MState newState) {

    if (state == MState::FINISHED) {
        reset();
    }

    switch (newState) {
    case MState::IDLE:
        setLabel(label_message, "");
        gtk_button_set_label(GTK_BUTTON(start_button), "Start Measurement");
        break;
    case MState::RESTARTING:
    case MState::MEASURING:
        gtk_button_set_label(GTK_BUTTON(start_button), "ABORT");
        break;
    case MState::FINISHED:
        gtk_button_set_label(GTK_BUTTON(start_button), "New Measurement");
        break;
    case MState::ERROR:
        gtk_button_set_label(GTK_BUTTON(start_button), "RETRY");
        break;
    }

    state = newState;
}

void GtkClient::activate(GtkApplication *app,
                         gpointer user_data) {
    auto self = reinterpret_cast<GtkClient *>(user_data);
    self->doActivate(app);
}

GtkWidget *GtkClient::staticLabel(const char *text, GtkAlign align) {
    GtkWidget *label = gtk_label_new(text);
    gtk_widget_set_halign(label, align);
    gtk_widget_set_name(label, "static-label");
    return label;
}

GtkWidget *GtkClient::dynamicLabel(GtkAlign align) {
    GtkWidget *label = gtk_label_new("");
    gtk_widget_set_halign(label, align);
    gtk_widget_set_name(label, "dynamic-label");
    return label;
}

void GtkClient::doActivate(GtkApplication *app) {
    main_window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(main_window), "Bandwidth Measurement");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 700, 300);

    GdkDisplay *display = gdk_display_get_default();
    GdkScreen *screen = gdk_display_get_default_screen(display);
    GtkCssProvider *cssProvider = gtk_css_provider_new();
    if (gtk_css_provider_load_from_path(cssProvider, "gtkclient.css", nullptr))
        gtk_style_context_add_provider_for_screen(screen,
                              GTK_STYLE_PROVIDER(cssProvider),
                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

    GtkWidget *mw = gtk_grid_new();
    gtk_widget_set_hexpand(mw, TRUE);
    gtk_grid_set_column_homogeneous(GTK_GRID(mw), TRUE);
    gtk_container_add(GTK_CONTAINER(main_window), mw);

    gtk_grid_set_row_spacing(GTK_GRID(mw), 10);
    gtk_grid_set_column_spacing(GTK_GRID(mw), 2);

    auto label_ticket1 = staticLabel("Suport ID: ");
    gtk_widget_set_halign(label_ticket1, GTK_ALIGN_END);
    gtk_grid_attach(GTK_GRID(mw), label_ticket1, 0, 0, 1, 1);
    label_ticket = dynamicLabel();
    gtk_grid_attach(GTK_GRID(mw), label_ticket, 1, 0, 1, 1);
    label_date = dynamicLabel();
    gtk_grid_attach(GTK_GRID(mw), label_date, 2, 0, 2, 1);

    auto label_isp1 = staticLabel("ISP: ");
    gtk_grid_attach(GTK_GRID(mw), label_isp1, 0, 1, 1, 1);
    label_isp = dynamicLabel();
    gtk_grid_attach(GTK_GRID(mw), label_isp, 1, 1, 3, 1);

    GtkWidget *label_ip1 = staticLabel("IP: ");
    gtk_grid_attach(GTK_GRID(mw), label_ip1, 0, 2, 1, 1);
    label_ip = dynamicLabel();
    gtk_grid_attach(GTK_GRID(mw), label_ip, 1, 2, 1, 1);

    GtkWidget *label_ip2 = staticLabel("Local IP: ");
    gtk_grid_attach(GTK_GRID(mw), label_ip2, 2, 2, 1, 1);
    label_pip = dynamicLabel();
    gtk_grid_attach(GTK_GRID(mw), label_pip, 3, 2, 1, 1);

    int res_line = 3;

    label_message = dynamicLabel(GTK_ALIGN_CENTER);
    gtk_grid_attach(GTK_GRID(mw), label_message, 0, res_line, 4, 1);

    ++res_line;

    GtkWidget *label_x2 = staticLabel("Latency: ");
    label_latency = dynamicLabel();
    GtkWidget *label_x3 = staticLabel("Evaluation: ");
    label_evaluation = dynamicLabel();

    gtk_grid_attach(GTK_GRID(mw), label_x2, 0, res_line, 1, 1);
    gtk_grid_attach(GTK_GRID(mw), label_latency, 1, res_line, 1, 1);
    gtk_grid_attach(GTK_GRID(mw), label_x3, 2, res_line, 1, 1);
    gtk_grid_attach(GTK_GRID(mw), label_evaluation, 3, res_line, 1, 1);

    ++res_line;

    GtkWidget *label_x0 = staticLabel("Download: ");
    label_download = dynamicLabel();
    GtkWidget *label_x1 = staticLabel("Upload: ");
    label_upload = dynamicLabel();
    gtk_grid_attach(GTK_GRID(mw), label_x0, 0, res_line, 1, 1);
    gtk_grid_attach(GTK_GRID(mw), label_download, 1, res_line, 1, 1);
    gtk_grid_attach(GTK_GRID(mw), label_x1, 2, res_line, 1, 1);
    gtk_grid_attach(GTK_GRID(mw), label_upload, 3, res_line, 1, 1);

    ++res_line;

    start_button = gtk_button_new_with_label("Start Measurement");
    gtk_grid_attach(GTK_GRID(mw), start_button, 1, res_line+4, 2, 1);
    g_signal_connect(start_button, "clicked",
                     G_CALLBACK(start_measurement), this);

    GtkWidget *label_server1 = staticLabel("Measurement server: ");
    gtk_grid_attach(GTK_GRID(mw), label_server1, 0, res_line+5, 1, 1);
    server_box = gtk_combo_box_text_new();
    gtk_grid_attach(GTK_GRID(mw), server_box, 1, res_line+5, 1, 1);

    iptype_box = gtk_combo_box_text_new();
    gtk_grid_attach(GTK_GRID(mw), iptype_box, 2, res_line+5, 1, 1);

    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(iptype_box),
                                   "ipv4");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(iptype_box),
                                   "ipv6");
    gtk_combo_box_set_active(GTK_COMBO_BOX(iptype_box), 0);
    g_signal_connect(iptype_box, "changed",
                     G_CALLBACK(update_serverlist), this);

    auto cond = static_cast<GIOCondition>(G_IO_IN | G_IO_HUP | G_IO_ERR);
    g_unix_fd_add(peer_fd, cond, poll_agent, this);

    gtk_widget_show_all(main_window);
}
