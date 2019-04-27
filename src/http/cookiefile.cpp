// Copyright (c) 2019 Internetstiftelsen
// Written by Göran Andersson <initgoran@gmail.com>

#include <fstream>
#include <iterator>

#include "cookiefile.h"
#include "../http/http_common.h"

CookieFile::~CookieFile() {
    if (isDirty())
        writeCookiesFile();
}

void CookieFile::readCookiesFile() {
    if (filename.empty())
        return;
    std::ifstream cookieFile(filename);
    if (!cookieFile) {
        // Failed, but who cares?
        log() << "cannot read cookie file " << filename;
        return;
    }

    log() << "reading cookie file " << filename;
    std::string domain;
    std::string line;
    auto it = cookies().end();
    while (std::getline(cookieFile, line)) {
        if (line.size() < 10) {
            continue;
        } else if (line[0] == '*') {
            if (line.substr(0, 5) == "**** " &&
                line.substr(line.size()-5) == " ****") {
                domain = line.substr(5, line.size()-10);
                it = cookies().find(domain);
            } else {
                err_log() << "bad cookie file";
                domain.clear();
            }
            continue;
        } else if (domain.empty()) {
            continue;
        } else {
            // name, value, domain, expires, path, secure, httponly
            std::vector<std::string> vec = HttpCommon::split(line, ";");
            if (vec.size() != Field::field_len)
                continue;
            if (it == cookies().end()) {
                std::vector<std::string> v(Cache::cache_len);
                auto p = cookies().insert(std::make_pair(domain, v));
                it = p.first;
            }
            auto &v = it->second;
            std::move(vec.begin(), vec.end(), back_inserter(v));
        }
    }

    clearDirty();
}

void CookieFile::writeCookiesFile() {
    if (filename.empty()) {
        log() << "missing name of cookie file";
        return;
    }

    std::ofstream cookieFile(filename, std::ofstream::trunc);
    for (auto p : cookies()) {
        const std::string &domain = p.first;
        const std::vector<std::string> &v = p.second;
        bool said_domain = false;
        std::size_t i=Cache::cache_len;
        while (i+Field::field_len <= v.size()) {
            if (v[i+Field::expires].empty() ||
                isExpired(v[i+Field::expires])) {
                // Session cookie or expired, don't persist.
                i += Field::field_len;
                continue;
            } else if (!said_domain) {
                cookieFile << "**** " << domain << " ****\n";
                said_domain = true;
            }
            cookieFile << v[i++];
            for (std::size_t j=1; j<Field::field_len; ++j)
                cookieFile << ';' << v[i++];
            cookieFile << '\n';
        }
    }
    cookieFile.close();

    if (cookieFile) {
        clearDirty();
    } else {
        // Failed, but who cares?
        log() << "cannot write cookie file";
    }
}
