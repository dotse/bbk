// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <map>
#include <set>
#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>

/// Exception thrown on syntax errors in task config
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

/// \brief
/// Read configuration from file or string
///
/// Empty lines are ignored.
///
/// A # character means the rest of the line is a comment.
///
/// All other lines must contain a configuration directive.
/// If the line contains a space, the directive ends at the first space,
/// and the rest of the line is the value of the directive.
///
/// Example:
///
///     logfile /var/log/my_service.log
///     name My Service  # Will be printed in greeting message
///
///     listen 80
///     listen 443 tls /etc/pki/tls/certs/mycert.pem /etc/pki/tls/private/mycert.pem
class TaskConfig {
public:
    /// Empty configuration.
    TaskConfig() {}

    /// Load configuration from file.
    TaskConfig(std::istream &cfg_stream);

    /// Load configuration from string.
    TaskConfig(const std::string &cfg_text);

    /// Add a directive to the config.
    void add(const std::string &key, const std::string &val);

    /// Replace value(s) of a directive with a new one.
    void set(const std::string &key, const std::string &val) {
        the_config.erase(key);
        add(key, val);
    }

    /// Remove value(s) of a directive.
    void erase(const std::string &key) {
        the_config.erase(key);
    }

    /// Set value of a directive unless already set.
    void setDefault(const std::string &key, const std::string &val) {
        if (the_config.find(key) == the_config.end())
            add(key, val);
    }

    /// Incrementally add to the config.
    void addLine(const std::string &line);

    /// Start iterator to loop over the config.
    std::multimap<std::string, std::string>::iterator begin() {
        return the_config.begin();
    }

    /// End iterator to loop over the config.
    std::multimap<std::string, std::string>::iterator end() {
        return the_config.end();
    }

    /// Start const iterator to loop over the config.
    std::multimap<std::string, std::string>::const_iterator begin() const {
        return the_config.begin();
    }

    /// End const iterator to loop over the config.
    std::multimap<std::string, std::string>::const_iterator end() const {
        return the_config.end();
    }

    /// Make a set of directives available to worker processes.
    void workerAttributes(const std::set<std::string> &attrs);

    /// Read config from file.
    static TaskConfig load(const std::string &filename);

    /// Return the parsed configuration.
    const std::multimap<std::string, std::string> &cfg() const {
        return the_config;
    }

    /// Return value of last occurence of key, or empty string.
    std::string value(const std::string &key) const;

    /// Return true if key exists, otherwise false:
    bool hasKey(const std::string &key) const {
        return the_config.find(key) != the_config.end();
    }

    /// Return a range of the key/value paris for the given key.
    std::pair<std::multimap<std::string, std::string>::const_iterator,
        std::multimap<std::string, std::string>::const_iterator>
        range(const std::string &key) const {
        return the_config.equal_range(key);
    }

    /// \brief
    /// Log to the file specified by the `logfile` directive.
    ///
    /// If the `logfile` key exists, and its value is not "-", try to use the
    /// value as a file name for the log.
    void openlog(std::ofstream &logger, bool append = false) const;

    /// \brief
    /// Split config value into non-blank strings.
    ///
    /// Return set of all non-whitespace strings listed after the configuration
    /// directive given by second parameter.
    std::set<std::string>
        parseList(const std::string &category = "whitelist") const;

    /// Parse command line arguments starting with "--":
    void parseArgs(int &argc, char **&argv);

    /// Return map of all key-value pairs of strings listed after the
    /// configuration directive given by second parameter.
    std::map<std::string, std::string>
        parseKeyVal(const std::string &category = "user") const;

    /// Store contents as a JSON object. Return false on failure.
    bool saveJsonToFile(const std::string &filename);

    /// \brief Load key/value pairs from JSON object.
    ///
    /// Values that are not strings will
    /// be ignored. Return empty object on failure.
    static TaskConfig loadJsonFromFile(const std::string &filename);

private:
    void _load(std::istream &cfg_stream);
    std::multimap<std::string, std::string> the_config;
};

std::ostream &operator<<(std::ostream &out, const TaskConfig &tc);
