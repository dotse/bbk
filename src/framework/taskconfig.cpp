// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#include <sstream>
#include <fstream>
#include "taskconfig.h"
#include "eventloop.h"

TaskConfig::TaskConfig(std::istream &cfg_stream) {
    _load(cfg_stream);
}

TaskConfig::TaskConfig(const std::string &cfg_text) {
    std::istringstream cfg_stream(cfg_text);
    _load(cfg_stream);
}
const char *BadTaskConfig::what() const noexcept {
    return err_msg.c_str();
}

BadTaskConfig::~BadTaskConfig() noexcept {
}

TaskConfig TaskConfig::load(const std::string &filename) {
    std::ifstream cfg_stream(filename);
    return TaskConfig(cfg_stream);
}

std::string TaskConfig::value(const std::string &key) const {
    auto range = the_config.equal_range(key);
    if (range.first == range.second)
        return std::string();
    --range.second;
    return range.second->second;
}

void TaskConfig::openlog(std::ofstream &logger, bool append) const {
    auto p = the_config.find("logfile");
    if (p != the_config.end() && p->second != "-") {
        logger.open(p->second, append ? std::ios::app : std::ios::trunc);
        if (logger) {
            Logger::setLogFile(logger);
#ifndef _WIN32
            EventLoop::setLogFilename(p->second);
#endif
        }
    }
}

void TaskConfig::add(const std::string &key, const std::string &val) {
    the_config.insert(std::make_pair(key, val));
}

void TaskConfig::addLine(const std::string &line) {
    size_t pos = line.find('#');
    if (pos != std::string::npos) {
        addLine(line.substr(0, pos));
        return;
    }
    if (line.empty())
        return;
    pos = line.find(" ");
    if (pos == std::string::npos)
        the_config.insert(std::make_pair(line, std::string()));
    else
        the_config.insert(std::make_pair(line.substr(0, pos),
                                         line.substr(pos+1)));
}

void TaskConfig::workerAttributes(const std::set<std::string> &attrs) {
    std::vector<std::string> to_add;
    for (auto &p : the_config)
        if (attrs.find(p.first) != attrs.end())
            to_add.push_back(p.first + " " + p.second);
    for (auto &line : to_add)
        add("workercfg", line);
}

void TaskConfig::_load(std::istream &cfg_stream) {
    std::string line;
    while (getline(cfg_stream, line)) {
        addLine(line);
    }
    if (!cfg_stream.eof())
        throw BadTaskConfig();
}

std::ostream &operator<<(std::ostream &out, const TaskConfig &tc) {
    out << "[ ";
    for (auto &p : tc)
        out << p.first << " --> " << p.second << " ";
    out << ']';
    return out;
}

std::set<std::string>
TaskConfig::parseList(const std::string &category) const {
    std::string val;
    std::set<std::string> res;
    auto to = the_config.upper_bound(category);
    for (auto p=the_config.lower_bound(category); p!=to; ++p) {
        std::istringstream s(p->second);
        while (s >> val)
            res.insert(val);
    }
    return res;
}

void TaskConfig::parseArgs(int &argc, char **&argv) {
    int apos = 0;
    while (++apos < argc) {
        std::string arg = argv[apos];
        if (arg.substr(0, 2) != "--")
            break;
        arg.erase(0, 2);
        if (arg.empty()) {
            ++apos;
            break;
        }
        auto pos = arg.find('=');
        if (pos == std::string::npos)
            the_config.insert(std::make_pair(arg, std::string()));
        else if (pos)
            the_config.insert(std::make_pair(arg.substr(0, pos),
                                             arg.substr(pos+1)));
        // else ignore
    }
    if (--apos) {
        argc -= apos;
        argv[apos] = argv[0];
        argv += apos;
    }
}

std::map<std::string, std::string>
TaskConfig::parseKeyVal(const std::string &category) const {
    std::string key, val;
    std::map<std::string, std::string> res;
    auto to = the_config.upper_bound(category);
    for (auto p=the_config.lower_bound(category); p!=to; ++p) {
        std::istringstream s(p->second);
        if (s >> key >> val)
            res.insert(std::make_pair(key, val));
    }
    return res;
}
