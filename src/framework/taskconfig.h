// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <map>
#include <set>
#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>

class BadTaskConfig : public std::exception {
public:
    BadTaskConfig(const std::string &msg = "cannot read config file") :
        err_msg(msg) {
    }
    const char *what() const noexcept override;
    BadTaskConfig(const BadTaskConfig &old) : err_msg(old.err_msg) {
    }
    ~BadTaskConfig() noexcept override;
    std::string err_msg;
};

// Empty lines are ignored.
// A # character means the rest of the line is a comment.
// All other lines must contain a configuration directive.
// If the line contains a space, the directive ends at the first space,
// and the rest of the line is the value of the directive.

class TaskConfig {
public:
    TaskConfig() {}
    TaskConfig(std::istream &cfg_stream);
    TaskConfig(const std::string &cfg_text);

    void add(const std::string &key, const std::string &val);
    void set(const std::string &key, const std::string &val) {
        the_config.erase(key);
        add(key, val);
    }
    void erase(const std::string &key) {
        the_config.erase(key);
    }
    void setDefault(const std::string &key, const std::string &val) {
        if (the_config.find(key) == the_config.end())
            add(key, val);
    }
    void addLine(const std::string &line);

    std::multimap<std::string, std::string>::iterator begin() {
        return the_config.begin();
    }
    std::multimap<std::string, std::string>::iterator end() {
        return the_config.end();
    }

    std::multimap<std::string, std::string>::const_iterator begin() const {
        return the_config.begin();
    }
    std::multimap<std::string, std::string>::const_iterator end() const {
        return the_config.end();
    }

    // Make attrs available to worker processes
    void workerAttributes(const std::set<std::string> &attrs);

    static TaskConfig load(const std::string &filename);
    const std::multimap<std::string, std::string> &cfg() const {
        return the_config;
    }
    // Return value of last occurence of key, or empty string.
    std::string value(const std::string &key) const;
    // Return true if key exists, otherwise false:
    bool hasKey(const std::string &key) const {
        return the_config.find(key) != the_config.end();
    }
    // Return a range of the key/value paris for the given key:
    std::pair<std::multimap<std::string, std::string>::const_iterator,
        std::multimap<std::string, std::string>::const_iterator>
        range(const std::string &key) const {
        return the_config.equal_range(key);
    }

    // If the "logfile" key exists, and its value is not "-", try to use the
    // value as a file name for the log.
    void openlog(std::ofstream &logger, bool append = false) const;

    // Return set of all non-whitespace strings listed after the configuration
    // directive given by second parameter.
    std::set<std::string>
        parseList(const std::string &category = "whitelist") const;

    // Parse command line arguments starting with "--":
    void parseArgs(int &argc, char **&argv);

    // Return map of all key-value pairs of strings listed after the
    // configuration directive given by second parameter.
    std::map<std::string, std::string>
        parseKeyVal(const std::string &category = "user") const;

    // Store contents as a JSON object. Return false on failure.
    bool saveJsonToFile(const std::string &filename);

    // Load key/value pairs from JSON object. Values that are not strings will
    // be ignored. Return empty object on failure.
    static TaskConfig loadJsonFromFile(const std::string &filename);

private:
    void _load(std::istream &cfg_stream);
    std::multimap<std::string, std::string> the_config;
};

std::ostream &operator<<(std::ostream &out, const TaskConfig &tc);
