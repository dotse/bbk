// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#define mkdir(x) _mkdir(x)
#else
#include <csignal>
#include <unistd.h>
#include <sys/stat.h>
#endif
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <fstream>
#include "utils.h"
#include "../framework/taskconfig.h"
#include "../measurement/defs.h"
#include "../http/sha1.h"
#include "../framework/logger.h"
#include <vector>

namespace {
#ifdef _WIN32
    std::string pathSep = "\\";
#else
    std::string pathSep = "/";
#endif
}

std::string createAndGetAppDir(std::string dir) {
    std::string home;
#ifdef _WIN32
    if (dir.empty()) {
        if (!std::getenv("HOMEDRIVE") || !std::getenv("HOMEPATH"))
            return "";
        home = std::string(std::getenv("HOMEDRIVE")) + std::getenv("HOMEPATH");
        dir = home + "\\.bredbandskollen";
    }
    _mkdir(dir.c_str());
#else
    if (dir.empty()) {
        if (!std::getenv("HOME"))
            return "";
        home = std::getenv("HOME");
        dir = home + "/.bredbandskollen";
    }
    int status = mkdir(dir.c_str(), 0755);
    if (status && errno != EEXIST) {
        return "";
    }
#endif

#ifdef IS_SANDBOXED
    if (!home.empty())
        chdir(home.c_str());
#endif

    return dir + pathSep;
}

enum class CliMode { NONE, LIVE, TEST, LOCAL,
#if defined(RUN_SERVER)
                     SERVER,
#endif
                     IN_ERROR } ;

bool parseArgs(int argc, char *argv[],
               TaskConfig &client_cfg, TaskConfig &agent_cfg) {

    CliMode mode = CliMode::NONE;

    client_cfg.set("port", "80");
    client_cfg.set("mtype", "ipv4");
    client_cfg.set("listen_addr", "127.0.0.1");

    agent_cfg.add("Measure.Webserver", "frontend.bredbandskollen.se");
    agent_cfg.add("Measure.SettingsUrl", "/api/servers");
    agent_cfg.add("Measure.ContentsUrl", "/api/content");
    agent_cfg.add("Measure.MeasurementsUrl", "/api/measurements");

    for (int i=1; i<argc; ++i) {
        std::string arg(argv[i]);
        if (arg == "--v6") {
            client_cfg.set("mtype", "ipv6");
            client_cfg.set("Measure.IpType", "override");
        } else if (arg == "--v4") {
            client_cfg.set("mtype", "ipv4");
            client_cfg.set("Measure.IpType", "override");
        } else if (arg == "--test") {
            mode = (mode == CliMode::NONE) ? CliMode::TEST : CliMode::IN_ERROR;
        } else if (arg == "--live") {
            mode = (mode == CliMode::NONE) ? CliMode::LIVE : CliMode::IN_ERROR;
        } else if (arg == "--version") {
            std::cout << measurement::appName << ' '
                      << measurement::appVersion << '\n';
            return false;
        } else if (arg == "--quiet") {
            client_cfg.set("quiet", "1");
        } else if (arg == "--local") {
            mode = (mode == CliMode::NONE) ? CliMode::LOCAL : CliMode::IN_ERROR;
#if defined(RUN_SERVER)
        } else if (arg == "--run-server") {
            mode = (mode == CliMode::NONE) ? CliMode::SERVER : CliMode::IN_ERROR;
#endif
        } else if (arg.substr(0, 11) == "--duration=") {
            agent_cfg.set("Measure.LoadDuration",  argv[i]+11);
            client_cfg.set("Measure.LoadDuration", "override");
        } else if (arg.substr(0, 13) == "--speedlimit=") {
            agent_cfg.set("Measure.SpeedLimit", argv[i]+13);
            client_cfg.set("Measure.SpeedLimit", "override");
        } else if (arg.substr(0, 6) == "--out=")
            client_cfg.set("out", argv[i]+6);
        else if (arg.substr(0, 6) == "--dir=")
            client_cfg.set("app_dir", (argv[i]+6) + pathSep);
        else if (arg.substr(0, 6) == "--log=")
            client_cfg.set("logfile", argv[i]+6);
        else if (arg.substr(0, 11) == "--local-ip=")
            agent_cfg.set("Measure.LocalAddress", argv[i]+11);
        else if (arg.substr(0, 9) == "--server=")
            client_cfg.set("server", argv[i]+9);
        else if (arg.substr(0, 7) == "--port=")
            client_cfg.set("port", argv[i]+7);
        else if (arg.substr(0, 12) == "--configure=")
            client_cfg.add("configure", argv[i]+12);
        else if (arg.substr(0, 9) == "--listen=")
            client_cfg.set("listen", argv[i]+9);
        else if (arg.substr(0, 14) == "--listen-addr=")
            client_cfg.set("listen_addr", argv[i]+14);
        else if (arg.substr(0, 12) == "--listen-pw=") {
            client_cfg.set("listen_pw", argv[i]+12);
#ifdef USE_GNUTLS
        } else if (arg == "--ssl") {
            agent_cfg.set("Measure.TLS", "1");
            client_cfg.set("ssl", "1");
            if (client_cfg.value("port") == "80")
                client_cfg.set("port", "443");
#endif
        } else if (arg.substr(0, 9) == "--fakeip=")
            agent_cfg.set("Client.fakeip", argv[i]+9);
        else if (arg == "--check-servers")
            client_cfg.set("pingsweep", "1");
        else if (arg.substr(0, 14) == "--measurements")
            client_cfg.set("list_measurements",
                (arg.size() > 15 && arg[14] == '=') ? argv[i]+15 : "10");
        else if (arg.substr(0, 10) == "--from-id=") {
            client_cfg.set("list_from", argv[i]+10);
            if (client_cfg.value("list_measurements").empty())
                client_cfg.set("list_measurements", "10");
        } else if (arg == "--browser") {
            client_cfg.set("browser", "1");
            if (client_cfg.value("listen").empty())
                client_cfg.set("listen", "0"); // Use any avaliable port
        } else if (arg.substr(0, 13) == "--proxy-host=")
            agent_cfg.set("Measure.ProxyServerUrl", argv[i]+13);
        else if (arg.substr(0, 13) == "--proxy-port=")
            agent_cfg.set("Measure.ProxyServerPort", argv[i]+13);
        else {
            int status = 0;
            if (arg != "--help") {
                status = 1;
                std::cerr << argv[0] << ": invalid argument -- " << arg << std::endl;
            }
            std::ostream &fh = status ? std::cerr : std::cout;
            fh << "Usage: " << argv[0] << " [OPTION]...\n\nOptions:\n\n"
                  "  --help              Show this help text\n"
                  "  --version           Print version number and exit\n"
               << "\nNetwork related options:\n"
#ifndef BBK_WEBVIEW
               << "  --v6                Prefer IPv6 (default is IPv4)\n"
#endif
#ifdef __linux__
#else
               << "  --local-ip=IP       Measure using existing local ip address IP\n"
               << "                      Note: this will not work on all platforms\n"
#endif
               << "  --proxy-host=HOST   Use HTTP proxy server HOST\n"
               << "  --proxy-port=PORT   Use port PORT on proxy server (default 80)\n"
               << "\nMeasurement configuration:\n"
#ifndef BBK_WEBVIEW
               << "  --server=HOST       Use HOST as measurement server\n"
               << "  --port=N            Port number for measurement server, default 80\n"
#endif
#ifdef USE_GNUTLS
               << "  --ssl               Measure using transport layer security (default port 443)\n"
#endif
               << "  --duration=N        Measure upload/download for N seconds (2-10, default 10)\n"
               << "  --speedlimit=N      Keep upload/download speed below N mbps on average\n"
               << "\nMeasurement type:\n"
               << "  --live              Measure using Bredbandskollen's live servers (default)\n"
               << "  --test              Measure using Bredbandskollen's development servers\n"
#ifndef BBK_WEBVIEW
               << "  --local             Don't fetch configuration (server list) from bredbandskollen.se,\n"
               << "                      communicate only with server given by the --server option.\n"
#endif
#if defined(RUN_SERVER)
               << "  --run-server        Run as a measurement server (requires option --listen=PORT)\n"
#endif
               << "\nLogging:\n"
               << "  --log=FILENAME      Write debug log to FILENAME\n"
               << "                      (log to stderr if FILENAME is -)\n"
#ifndef BBK_WEBVIEW
               << "\nFinding measurement servers:\n"
               << "  --check-servers     Find closest measurement server\n"
               << "\nList previous measurements:\n"
               << "  --measurements      List 10 last measurements\n"
               << "  --measurements=N    List N last measurements\n"
               << "                      If --quiet, output will be JSON. Otherwise\n"
               << "                      output will be lines with tab separated fields.\n"
               << "  --from-id=N         List only measurements before ID N\n"
               << "\nBrowser interface:\n"
               << "  --browser           Use a web browser as interface\n"
               << "  --listen=PORT       Use web browser as interface;\n"
               << "                      the browser must connect to the given PORT\n"
               << "  --listen-addr=IP    When listening, bind socket to ip address IP\n"
               << "                      (default is 127.0.0.1) to use a web browser on\n"
               << "                      a remote host as interface\n"
               << "                      Note: this may not work due to e.g. firewalls.\n"
               << "                      Don't use it unless you know what you are doing.\n"
               << "  --listen-pw=PW      Use PW as a one-time password when connecting from browser\n"
               << "                      Note: DO NOT reuse a sensitive password here!\n"
               << "                      It is better to omit this option because by default\n"
               << "                      a secure one-time password will be generated.\n"
               << "\nCommand line interface:\n"
               << "  --quiet             Write a single line of output\n"
               << "  --out=FILENAME      Append output to FILENAME instead of stdout\n"
#endif
               << std::endl;
            return false;
        }
    }

    client_cfg.set("app_dir", createAndGetAppDir(client_cfg.value("app_dir")));

    if (client_cfg.value("local") == "1" && client_cfg.value("server").empty()) {
        std::cerr << "missing --server option" << std::endl;
        return false;
    }

    std::vector<std::string> pdir = { "listen", "port" };
    for (auto &str : pdir)
        if (!client_cfg.value(str).empty()) {
            auto port = client_cfg.value(str);
            if (port.find_first_not_of("0123456789") != std::string::npos ||
                port.size() > 5 || std::stod(port) > 65535) {
                std::cerr << "invalid port number" << std::endl;
                return false;
            }
        }

    switch (mode) {
    case CliMode::NONE:
    case CliMode::LIVE:
        agent_cfg.set("Measure.Webserver", "frontend.bredbandskollen.se");
        break;
    case CliMode::TEST:
        agent_cfg.set("Measure.Webserver", "frontend-beta.bredbandskollen.se");
        break;
    case CliMode::LOCAL:
        client_cfg.set("local", "1");
        agent_cfg.set("Measure.Webserver", "none");
        break;
#if defined(RUN_SERVER)
    case CliMode::SERVER:
        client_cfg.set("local", "1");
        client_cfg.set("run_server", "1");
        if (client_cfg.value("listen").empty()) {
            std::cerr << "option --listen is required with --run-server"
                      << std::endl;
            return false;
        }
        break;
#endif
    case CliMode::IN_ERROR:
        std::cerr << "can have only one of options --live, --test,";
#if defined(RUN_SERVER)
        std::cerr << " --run-server,";
#endif
        std::cerr << " and --local";
        return false;
    }

    if (!client_cfg.value("listen").empty() &&
        client_cfg.value("listen_pw").empty()) {
        client_cfg.add("listen_pw", Logger::createHashKey(12));
    }
    client_cfg.add("url", "http://" + agent_cfg.value("Measure.Webserver") +
                   "/standalone/dev/index.php");

    if (client_cfg.value("logfile").empty()) {
#if defined(RUN_SERVER)
        if (mode == CliMode::SERVER)
            client_cfg.add("logfile", client_cfg.value("app_dir") + "server_log");
        else
#endif
            client_cfg.add("logfile", client_cfg.value("app_dir") + "last_log");
    }
    client_cfg.set("config_file", client_cfg.value("app_dir") + "config");
    agent_cfg.set("options_file", client_cfg.value("app_dir") + "ConfigOptions.txt");

    // Default to ipv6 if user wants to use a local ipv6 address
    if (agent_cfg.value("Measure.LocalAddress").find(':') !=
        std::string::npos) {
        client_cfg.set("mtype", "ipv6");
        client_cfg.set("Measure.IpType", "override");
    }

    agent_cfg.add("Measure.AutoSaveReport",
                  client_cfg.value("listen").empty() ? "false" : "true");

    agent_cfg.add("Measure.IpType", client_cfg.value("mtype"));

    agent_cfg.add("Client.appname", measurement::appName);
    agent_cfg.add("Client.appver", measurement::appVersion);
    agent_cfg.add("Client.machine", measurement::hw_info);
    agent_cfg.add("Client.system", measurement::os_version);
    agent_cfg.add("Client.language", "en");

    return true;
}
