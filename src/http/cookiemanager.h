// Copyright (c) 2019 The Swedish Internet Foundation
// Written by Göran Andersson <goran@init.se>

// This class stores cookies in memory only.
// Create a subclass if you want persistent cookies.

#pragma once

#include <map>
#include <vector>

#include "../framework/logger.h"

class CookieManager : public Logger {
public:
    CookieManager(const std::string &name = "CookieManager") :
        Logger(name) {
    }

    virtual ~CookieManager();

    void setCookie(const std::string &line, std::string domain,
                   std::string uri);

    // Return empty string if no cookies exist, or a HTTP header line
    //   "Cookie: name1=val1; name2=val2\r\n"
    std::string httpHeaderLine(const std::string &domain,
                               const std::string &uri);

    // Return cookie value for domain and name; return empty if not found.
    std::string getCookieVal(const std::string &name, std::string domain);

    // Return true if no cookies have been set.
    bool empty() const {
        return store.empty();
    }

    void eraseCookies(const std::string &domain);

    // Store persistently, return true on success
    virtual bool save();

protected:
    bool isDirty() const {
        return dirty;
    }
    void clearDirty() {
        dirty = false;
    }

    // timeval contains only digits 0-9, doesn't start with 0.
    bool isExpired(const std::string &timeval);

    // both must be non-empty, only digits, no leading zeroes.
    static bool less(const std::string &tv1, const std::string &tv2) {
        if (tv1.size() < tv2.size())
            return true;
        if (tv1.size() > tv2.size())
            return false;
        return tv1 < tv2;
    }

    // Access to all cookies, to serialise/deserialise.
    // The key is the domain for which the cookie is valid (also subdomains).
    // The value is a vector with elements as follows:
    // The first Cache::cache_len elements are used to cache values and must be
    // ignored. The remaining elements are 0 or more groups of Field::field_len
    // elements; each group describes the properties of a single cookie.
    std::map<std::string, std::vector<std::string> > &cookies() {
        return store;
    }
    enum Cache {
        cookie_header, expiry, cache_len
    };
    enum Field {
        name, value, domain, expires, path, secure, httponly, field_len
    };
private:
    // Map domain to cookie strings:
    // First element of each store vector is a cache for httpHeaderLine():
    //   - empty string means nothing is cached
    //   - string of length 1 means there are no cookies for the domain
    //   - string of length > 1 is the cookie header line
    //   - NOTE! We only cache for path=/
    // Second element is cache expiry time, empty for no expiry.
    std::map<std::string, std::vector<std::string> > store;
    // TODO: Store time of creation / last use?
    //       last use may ignore cache.

    static std::vector<std::string> cookieSplit(const std::string &line);
    TimePoint expire_date;
    bool dirty = false;
    time_t t_now = 0;
    std::string s_now = "0";
};
