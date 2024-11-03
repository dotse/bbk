// Copyright (c) 2017 The Swedish Internet Foundation
// Written by Göran Andersson <goran@init.se>

#pragma once

#include <cstdint>
#include <string>
#include <cstdint>

class CookieManager;

/// \brief
/// The host name and port number of a HTTP host.
///
/// May also contain a pointer to a cookie jar.
/// The same CookieManager should
/// be used for all objects with the same hostname.
class HttpHost {
public:
    /// You create and own the cookie manager.
    HttpHost(const std::string &hName = std::string(),
             uint16_t sPort = 80,
             const std::string &pHost = std::string(),
             uint16_t pPort = 0,
             CookieManager *cMgr = nullptr) :
        hostname(hName), proxyHost(pHost),
        port(sPort), proxyPort(pPort), cmgr(cMgr) {
#ifdef USE_GNUTLS
        is_tls = (sPort == 443);
#endif
    }
    HttpHost(const char *hName, uint16_t sPort = 80) :
        hostname(hName), port(sPort) {
#ifdef USE_GNUTLS
        is_tls = (sPort == 443);
#endif
        cmgr = nullptr;
    }
    std::string hostname;
    std::string proxyHost;
    uint16_t port;
    uint16_t proxyPort;
    CookieManager *cmgr;
    uint16_t iptype = 0; // 4 for ipv4, 6 for ipv6, 0 for any.
#ifdef USE_GNUTLS
    bool is_tls;
#endif
};
