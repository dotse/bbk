// Copyright (c) 2019 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#include "cookiemanager.h"
#include "http_common.h"
#include <algorithm>
#include <stdexcept>
#include <iterator>

CookieManager::~CookieManager() {
}

bool CookieManager::save() {
    return false;
}

// timeval contains only digits 0-9, doesn't start with 0.
bool CookieManager::isExpired(const std::string &timeval) {
    if (timeval.empty())
        return false; // No expiry - a session cookie.

    if (time_t d = time(nullptr) - t_now) {
        t_now += d;
        s_now = std::to_string(t_now);
    }

    return !less(s_now, timeval);
}

// line is the remainder of a header line starting with "Set-Cookie: ", e.g.
//   previous_isp=23; expires=Wed, 07-Jun-2017 11:34:59 GMT; path=/;
//   domain=.bredbandskollen.se
void CookieManager::setCookie(const std::string &line,
                              std::string domain, std::string uri) {
    std::vector<std::string> cookie_av = cookieSplit(line);
    if (cookie_av.empty())
        return;

    const std::string &cdomain = cookie_av[Field::domain];
    // Disallow third-party cookies and cookies for top-level domains.
    // TODO: should disallow all public suffixes, e.g. ".co.uk".
    if (cdomain.find('.') != std::string::npos &&
        HttpCommon::isSubdomain(domain, cdomain))
        domain = cdomain;
    auto it = store.find(domain);
    if (it == store.end()) {
        std::vector<std::string> v(Cache::cache_len);
        auto p = store.insert(std::make_pair(domain, v));
        it = p.first;
    } else {
        // Invalidate cache, also for subdomains.
        it->second[Cache::cookie_header].clear();
        for (auto p : store)
            if (HttpCommon::isSubdomain(p.first, domain))
                p.second[Cache::cookie_header].clear();
    }

    auto &v = it->second;
    bool expired = isExpired(cookie_av[Field::expires]);
    if (!expired)
        if (cookie_av[Field::path].empty() || cookie_av[Field::path][0] != '/')
            cookie_av[Field::path] = HttpCommon::uriPath(uri); // default-path

    // Check if the new cookie replaces an existing
    for (size_t i=Cache::cache_len; i<v.size(); i+=Field::field_len) {
        if (v[i+Field::name] == cookie_av[Field::name] &&
            v[i+Field::path] == cookie_av[Field::path]) {
            if (expired) {
                v.erase(v.begin() + static_cast<long>(i),
                        v.begin() + static_cast<long>(i+Field::field_len));
            } else {
                std::move(cookie_av.begin(), cookie_av.end(),
                          v.begin() + static_cast<long>(i));
                dirty = true;
                cookie_av.clear();
                return;
            }
            break;
        }
    }
    if (!expired) {
        std::move(cookie_av.begin(), cookie_av.end(),
                  back_inserter(v));
        dirty = true;
    }
}

// line is the remainder of a header line starting with "Set-Cookie: ", e.g.
//   previous_isp=23; expires=Wed, 07-Jun-2017 11:34:59 GMT; path=/;
//   domain=.bredbandskollen.se

// Return vector with Field::field_len elements if OK:
//   name, value, domain, expires, path, secure, httponly
// Return empty vector if bad.
std::vector<std::string> CookieManager::cookieSplit(const std::string &line) {
    std::vector<std::string> v = HttpCommon::split(line, "; "), cookie_av;
    for (auto av : v) {
        std::string lcname, value, avlc;
        auto eqpos = av.find('=');
        if (!cookie_av.empty()) {
            // Make a lower-case copy:
            std::transform(av.begin(), av.end(), back_inserter(avlc), tolower);
            HttpCommon::trimWSP(lcname = avlc.substr(0, eqpos));
            if (eqpos != std::string::npos)
                HttpCommon::trimWSP(value = av.substr(eqpos+1));
        } else {
            if (eqpos == std::string::npos)
                break; // Bad line
        }

        if (cookie_av.empty()) {
            cookie_av.resize(Field::field_len);
            HttpCommon::trimWSP(cookie_av[Field::name] = av.substr(0, eqpos));
            HttpCommon::trimWSP(cookie_av[Field::value] = av.substr(eqpos+1));
        } else if (eqpos == 6 && lcname == "domain") {
            ++eqpos;
            if (av[eqpos] == '.')
                ++eqpos; // Ignore leading dot.
            cookie_av[Field::domain] = avlc.substr(eqpos);
        } else if (eqpos == 8 && lcname == "_expires") {
            // Our internal format, unix timestamp value.
            ++eqpos;
            if (av.find_first_not_of("0123456789", eqpos) == std::string::npos)
                cookie_av[Field::expires] = av.substr(eqpos);
            else // Corrupt cookie file?
                cookie_av[Field::expires] = "0";
        } else if (eqpos == 7 && lcname == "expires") {
            // max-age has precedence, so ignore if already set.
            if (cookie_av[Field::expires].empty()) {
                time_t val = HttpCommon::parseDateRfc1123(av.substr(eqpos+1));
                if (val >= 0) {
                    cookie_av[Field::expires] = std::to_string(val);
                } else if (val == -1) {
                    // Ignore bad date
                } else {
                    // Expired.
                    cookie_av[Field::expires] = "0";
                }
            }
        } else if (eqpos == 4 && lcname == "path") {
            cookie_av[Field::path] = av.substr(eqpos+1);
        } else if (eqpos == 7 && lcname == "max-age") {
            ++eqpos;
            if (av.size() == eqpos) {
                // Ignore?
            } else if (av[eqpos] == '-') {
                if (av.find_first_not_of("0123456789", ++eqpos)
                    == std::string::npos)
                    cookie_av[Field::expires] = "0"; // Mark as expired
                // Else ignore!
            } else if (av.find_first_not_of("0123456789", eqpos) ==
                       std::string::npos) {
                time_t res, maxv = std::numeric_limits<time_t>::max();
                try {
                    long secs = std::stol(av.substr(eqpos));
                    res = time(nullptr);
                    if (secs >= maxv-res)
                        res = maxv;
                    else
                        res += secs;
                } catch (std::out_of_range &) {
                    // Too large
                    res = maxv;
                }
                cookie_av[Field::expires] = std::to_string(res);
            }
        } else if (avlc == "secure") {
            cookie_av[Field::secure] = avlc;
        } else if (avlc == "httponly") {
            cookie_av[Field::secure] = avlc;
        } else {
            // Unknown attribute; ignore.
        }
    }
    return cookie_av;
}

std::string CookieManager::httpHeaderLine(const std::string &domain,
                                          const std::string &uri) {
    auto cit = store.find(domain);
    if (cit == store.end()) {
        // Make room for cache!
        std::vector<std::string> v(Cache::cache_len);
        auto p = store.insert(std::make_pair(domain, v));
        cit = p.first;
    } else {
        // Check if cached value exists and hasn't expired:
        const std::vector<std::string> &v = cit->second;
        if (!v.empty() && !v[Cache::cookie_header].empty() &&
            !isExpired(v[Cache::expiry])) {
            // Cached value exists, no part of it has expired.
            if (v[Cache::cookie_header].size() == 1)
                return "";
            else
                return v[Cache::cookie_header];
        }
    }

    // We'll set this to true if we can't cache the result, e.g.
    // if it depends on path:
    bool dont_cache = false;

    // Find all subdomains for which we have cookies:
    std::vector<std::map<std::string, std::vector<std::string>>::iterator>
        subdomains;
    std::string subdomain = domain;
    auto it = cit;
    while (true) {
        if (it != store.end())
            subdomains.push_back(it);
        auto pos = subdomain.find('.');
        if (pos == std::string::npos)
            break; // We don't accept top level domains.
        subdomain.erase(0, pos+1);
        it = store.find(subdomain);
    }

    // Get all cookie values, starting from least significant subdomain:
    std::string cache_expiry;
    std::map<std::string, std::string> cookieVal;
    while (!subdomains.empty()) {
        // Reevaluate:
        auto &v = subdomains.back()->second;
        for (size_t i=Cache::cache_len; i<v.size(); ) {
            const std::string &expiry = v[i+Field::expires];
            if (!expiry.empty() &&
                (cache_expiry.empty() || less(expiry, cache_expiry))) {
                if (isExpired(expiry)) {
                    v.erase(v.begin()+static_cast<long>(i),
                            v.begin()+static_cast<long>(i+Field::field_len));
                    continue;
                } else
                    cache_expiry = expiry;
            }
            if (v[i+Field::path] != "/") {
                dont_cache = true;
                if (HttpCommon::isWithinPath(uri, v[i+Field::path]))
                    cookieVal[v[i+Field::name]] = v[i+Field::value];
            } else
                cookieVal[v[i+Field::name]] = v[i+Field::value];
            i += Field::field_len;
        }
        subdomains.pop_back();
    }

    auto p = cookieVal.begin();
    if (p == cookieVal.end()) {
        cit->second[Cache::cookie_header] = "-";
        cit->second[Cache::expiry].clear();
        return "";
    }
    std::string cval = "Cookie: " + p->first + "=" + p->second;
    while (++p != cookieVal.end()) {
        cval += ("; " + p->first + "=" + p->second);
    }

    cval += "\r\n";
    if (!dont_cache) {
        cit->second[Cache::cookie_header] = cval;
        cit->second[Cache::expiry] = cache_expiry;
    }
    return cval;
}

void CookieManager::eraseCookies(const std::string &domain) {
    auto cit = store.find(domain);
    if (cit == store.end())
        return;
    store.erase(cit);
    dirty = true;
}

std::string CookieManager::getCookieVal(const std::string &name,
                                        std::string domain) {
    while (true) {
        auto cit = store.find(domain);
        if (cit != store.end()) {
            const auto &v = cit->second;
            for (unsigned i=Cache::cache_len; i<v.size(); i+=Field::field_len)
                if (v[i+Field::name] == name && v[i+Field::path] == "/")
                    return v[i+Field::value];
        }
        auto pos = domain.find('.');
        if (pos == std::string::npos)
            return std::string();
        domain.erase(0, pos+1);
    }
}
