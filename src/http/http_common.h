// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <string>
#include <cstring>
#include <ctime>
#include <vector>
#include <map>

class HttpCommon {
public:
    // Return true on success.
    static bool parseHeaders(const std::string &header, std::string &r,
                             std::multimap<std::string, std::string> &res);

    // Guess mime type, e.g. "text/html", from url/filename
    static const char *mime_type(const std::string &file_name);

    // Return -1 on failure.
    static time_t parseDateRfc1123(const std::string &date);

    // Remove leading and trailing whitespace:
    static void trimWSP(std::string &s);

    // Split s into fields sepatared by sep:
    static std::vector<std::string> split(const std::string &s,
                                          const std::string &sep);
    static bool isSubdomain(const std::string &subdomain,
                            const std::string &domain) {
        auto s = domain.size();
        return (subdomain.size() > s &&
                subdomain[subdomain.size()-s-1] == '.' &&
                subdomain.substr(subdomain.size()-s) == domain);
    }
    static bool isWithinPath(const std::string &uri,
                             const std::string &path) {
        if (path.empty())
            return (!uri.empty() && uri[0] == '/');
        if (path != uri.substr(0, path.size()))
            return false;
        if (path.size() < uri.size()) {
            return path[path.size()-1] == '/' ||
                uri[path.size()] == '/';
        } else if (path.size() == uri.size()) {
            return true; // Identical.
        } else {
            return false;
        }
    }
    // Return the path part of an uri.
    static std::string uriPath(std::string uri) {
        auto qpos = uri.find('?');
        if (qpos != std::string::npos)
            uri.erase(qpos);
        qpos = uri.rfind('/');
        if (qpos != std::string::npos)
            uri.erase(qpos+1);
        return uri;
    }
};
