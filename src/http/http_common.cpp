// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#include "http_common.h"
#include <stdlib.h>
#include <time.h>

bool HttpCommon::parseHeaders(const std::string &header, std::string &r,
                              std::multimap<std::string, std::string> &res) {
    std::string::size_type start, end=0;
    while (true) {
        start = end;
        end = header.find("\r\n", start);
        if (end == std::string::npos)
            return false;
        std::string line = header.substr(start, end-start);
        if (line.empty()) {
            break;
        } else {
            // Remove trailing spaces
            std::size_t n = line.find_last_not_of(' ');
            if (n != std::string::npos)
                line.resize(n+1);
        }
        end += std::strlen("\r\n");

        if (!start) {
            r = line;
            continue;
        }
        std::string::size_type separator = line.find(": ");
        if (separator == std::string::npos)
            return false;
        std::string attribute = line.substr(0, separator);
        for (std::string::size_type i=0; i<attribute.size(); ++i)
            if (attribute[i] >= 'A' && attribute[i] <= 'Z')
                attribute[i] += ('a' - 'A');
        res.insert(std::make_pair(attribute, line.substr(separator+2)));
    }
    return true;
}

namespace {
    std::map<std::string, const char *> mime_db = {
        { "txt", "text/plain; charset=utf-8" },
        { "html", "text/html; charset=utf-8" },
        { "css", "text/css" },
        { "js", "text/javascript" },
        { "pdf", "application/pdf" },
        { "xml", "application/xml" },
        { "json", "application/json" },
    };
}

const char *HttpCommon::mime_type(const std::string &file_name) {
    auto pos = file_name.find_last_of('.');
    if (pos == std::string::npos)
        pos = 0;
    else
        ++pos;
    auto p = mime_db.find(file_name.substr(pos));
    if (p != mime_db.end())
        return p->second;
    return "application/octet-stream";
}

namespace {
    inline int toInt(char p1, char p2) {
        if (!isdigit(p1) || !isdigit(p2))
            return -1;
        return 10*(p1-'0') + (p2-'0');
    }
}

// Return -1 on failure.
time_t HttpCommon::parseDateRfc1123(const std::string &date) {
    time_t ret = -1;

    // Date format:
    // Wed, 07-Jun-2017 11:34:59 GMT
    // 012345678901234567890123456789

    if (date.size() != 29)
        return ret;
    static const std::string wdays("Sun, Mon, Tue, Wed, Thu, Fri, Sat, ");
    auto wday_pos = wdays.find(date.substr(0, 5));
    if (wday_pos == std::string::npos || wday_pos%5)
        return ret;

    static const std::string months("-Jan-Feb-Mar-Apr-May-Jun-Jul-Aug-Sep-Oct-Nov-Dec-");
    auto mon_pos = months.find(date.substr(7, 5));
    if (mon_pos == std::string::npos || mon_pos%4)
        return ret;

    if (date[16] != ' ' || date[19] != ':' || date[22] != ':' ||
        date.substr(25) != " GMT")
        return ret;

    struct tm timespec;
    timespec.tm_sec = toInt(date[23], date[24]);
    timespec.tm_min = toInt(date[20], date[21]);
    timespec.tm_hour = toInt(date[17], date[18]);
    int mday = toInt(date[5], date[6]);
    timespec.tm_mday = mday;
    if (timespec.tm_sec < 0 || timespec.tm_sec > 59 ||
        timespec.tm_min < 0 || timespec.tm_min > 59 ||
        timespec.tm_hour < 0 || timespec.tm_hour > 23 || mday < 0)
        return ret;
    timespec.tm_mon = static_cast<int>(mon_pos / 4);
    timespec.tm_year = toInt(date[14], date[15]);
    int yy = toInt(date[12], date[13]) - 19;
    if (timespec.tm_year < 0 || yy < 0)
        return ret;
    timespec.tm_year += yy*100;
    //timespec.tm_wday = 0;
    //timespec.tm_yday = 0;
    timespec.tm_isdst = -1;

    // mktime depends on current timezone, so we must
    // temporarily reset timezone.
    char *tz = getenv("TZ");
#ifdef _WIN32
    _putenv("TZ=UTC");
    _tzset();
    ret = mktime(&timespec);
    if (tz) {
        char buf[255];
        snprintf(buf, sizeof(buf), "TZ=%s", tz);
        _putenv(buf);
    } else {
        _putenv("TZ=");
    }
    _tzset();
#else
    setenv("TZ", "", 1);
    tzset();
    ret = mktime(&timespec);
    if (tz)
        setenv("TZ", tz, 1);
    else
        unsetenv("TZ");
    tzset();
    if (timespec.tm_wday*5 != static_cast<int>(wday_pos)
        || timespec.tm_mday != mday)
        return -1;
#endif

    return ret;
}

void HttpCommon::trimWSP(std::string &s) {
    auto pos = s.find_first_not_of(" \t\r\n\v");
    if (pos && pos != std::string::npos)
        s.erase(0, pos);

    pos = s.find_last_not_of(" \t\r\n\v");
    if (pos != std::string::npos)
        s.erase(++pos);
}

std::vector<std::string> HttpCommon::split(const std::string &s,
                                           const std::string &sep) {
    std::vector<std::string> v;
    std::string::size_type pos = 0;
    while (true) {
        auto epos = s.find(sep, pos);
        v.push_back(s.substr(pos, epos-pos));
        if (epos == std::string::npos)
            break;
        pos = epos + sep.size();
    }
    return v;
}
