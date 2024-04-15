// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#include <sstream>
#include <fstream>
#include <iostream>
#include "taskconfig.h"
#include "eventloop.h"
#include "../json11/json11.hpp"

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

/*Preconditions: New configs set with method "saveConfigurationOption"
Old config options loaded into the_config in MeasurementAgent() (loadJsonFromFile).
Postconditions: Configuration options transferred onto file */
bool TaskConfig::saveJsonToFile(const std::string &filename) {
    std::fstream cfgOptionsFile;

    //Create file if not existing. Write the updated config in json-format to file.
    cfgOptionsFile.open(filename, std::fstream::out);
    if (cfgOptionsFile.is_open()) {
        cfgOptionsFile << json11::Json(the_config).dump();
    } else {
        Logger::log("TaskConfig") << "Unable to open config file";
    }

    cfgOptionsFile.close();
    return bool(cfgOptionsFile);
}

/*
Used to load preexisting configuration options from file into json-object for further use.
*/
TaskConfig TaskConfig::loadJsonFromFile(const std::string &filename){
    std::string fileContent;
    std::ifstream inFile;
    std::string err;
    TaskConfig cfg;

    inFile.open(filename);

    if (inFile.is_open() && inFile) {
        //consume entire inFile, from beginning to end.
        fileContent.assign( (std::istreambuf_iterator<char>(inFile)),
                                 (std::istreambuf_iterator<char>()) );
    } else {
        Logger::log("TaskConfig") << "Unable to open config file";
    }

    json11::Json JsonObj = json11::Json::parse(fileContent, err);

    inFile.close();

    for (auto p : JsonObj.object_items()) {
        cfg.add(p.first, p.second.string_value());
    }

    return cfg;
}
