// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#ifdef _WIN32
#define NOMINMAX
#endif

#include <iomanip>
#include <cctype>
#include <stdexcept>

#include "httpclientconnection.h"
#include "httpclienttask.h"
#include "http_common.h"
#include "sha1.h"

HttpClientConnection::HttpClientConnection(const std::string &label,
                                           HttpClientTask *task,
                                           const std::string &hostname,
                                           uint16_t port,
                                           const std::string &http_hostname,
                                           uint16_t iptype,
                                           struct addrinfo *local_addr) :
    HttpConnection(label, task, hostname, port, iptype, local_addr),
    real_hostname(http_hostname.empty() ? hostname : http_hostname),
    owner_task(task) {
    if (!http_hostname.empty())
        proto_hostname = "http://" + http_hostname;
}

PollState HttpClientConnection::connected() {
    owner_task->newRequest(this);
    if (current_request == "pass")
        return PollState::NONE;
    if (current_request.empty())
        return PollState::KEEPALIVE;
    //dbg_log() << "HttpClientConnection::connected() request: " << current_request;
    asyncSendData(current_request);
    if (bytes_left_to_post)
        return PollState::WRITE;
    else
        return PollState::READ;
}

bool HttpClientConnection::parseChunkedEncodingLength(char *&buf, size_t &len) {
    // Between chunks, there is \r\n, a hex number, and \r\n again.
    // The number gives the size of next chunk.
    // The last size is 0, and then an extra \r\n to terminate the request.

    size_t oldlen = chunked_info.size();
    chunked_info += std::string(buf, len<20 ? len : 20);

    if (final_chunk) {
        len = 0;
        // We're waiting for the trailing \r\n.
        if (chunked_info == "\r\n") {
            // OK, all done!
            response_is_chunked = false;
            return true;
        }
        return (chunked_info == "\r");
    }

    if (chunked_info.size() < 5) {
        // Wait for more, keeping the fragment in chunked_info
        len = 0;
        return true;
    }
    if (chunked_info.substr(0, 2) != "\r\n")
        return false;

    size_t endOfSizePos = chunked_info.find("\r\n", 2);
    if (endOfSizePos == std::string::npos) {
        // Haven't got terminating \r\n yet.
        // Wait for more if len < 20, otherwise fail.
        len = 0;
        return chunked_info.size() < 20;
    }

    size_t to_skip = endOfSizePos + 2 - oldlen;
    buf += to_skip;
    len -= to_skip;

    try {
        std::size_t count;
        std::string num = "0x"+ chunked_info.substr(2);
        bytes_left_to_read = std::stoul(num, &count, 16);
        if (count != endOfSizePos)
            return false;
    } catch (...) {
        return false;
    }

    if (!bytes_left_to_read) {
        final_chunk = true;
    }

    chunked_info.clear();
    return true;
}

PollState HttpClientConnection::readData(char *buf, size_t len) {
    if (dbgIsOn()) {
        log() << socket() << "HttpClientConnection  <" << std::string(buf, len) << ">";
    }
    if (!response_header_complete) {
        std::string::size_type old_length = response_header.size();
        std::string::size_type to_append = len;
        if (old_length + to_append > max_header_length)
            to_append = max_header_length - old_length;
        response_header.append(buf, to_append);
        std::string::size_type pos = response_header.find("\r\n\r\n");
        if (pos == std::string::npos) {
            if (response_header.size() >= max_header_length)
                return PollState::KILL;
            else
                return PollState::READ;
        }
        response_header.resize(pos + strlen("\r\n\r\n"));
        if (!parseResponseHeaders())
            return PollState::CLOSE;
        len -= (response_header.size() - old_length);
        if (len)
            buf += (response_header.size() - old_length);
        else if (!response_header_complete) {
            response_header.clear();
            http_response_headers.clear();
            return PollState::READ; // e.g. after PROXY CONNECT
        }
    }

    if (is_websocket)
        return wsReadData(buf, len);

    if (dbgIsOn()) {
        dbg_log() << socket() << " HEADER OK, PAYLOAD <"
                  << std::string(buf, len) << "> LEFT: "
                  << bytes_left_to_read << " CH: " << response_is_chunked;
    }
    // Reading payload
    if (len) {
        if (response_is_chunked && !bytes_left_to_read) {
            if (!parseChunkedEncodingLength(buf, len)) {
                err_log() << "content encoding error";
                return PollState::KILL;
            }
        }
        if (bytes_left_to_read) {
            size_t to_read = std::min(len, bytes_left_to_read);
            if (buffer_response)
                buffer.append(buf, to_read);
            else
                owner_task->payload(this, buf, to_read);
            bytes_left_to_read -= to_read;
            len -= to_read;
            buf += to_read;
        }
    }

    if (response_is_chunked) {
        if (len > 0)
            return readData(buf, len);
        else
            return PollState::READ;
    }

    if (bytes_left_to_read > 0)
        return PollState::READ;

    bool to_continue = owner_task->requestComplete(this);

    if (len) {
        // Error
        err_log() << "got " << len << " bytes after request complete";
        return PollState::KILL;
    }
    if (to_continue) {
        current_request.clear();
        current_uri.clear();
        response_header.clear();
        response_header_complete = false;
        response_http_status = 0;
        buffer.clear();
        http_response_headers.clear();
        return connected(); // New request or close.
    } else {
        return PollState::KEEPALIVE;
    }
}

bool HttpClientConnection::parseResponseHeaders() {
    if (response_http_status) {
        err_log() << "internal error: headers already parsed";
        return false;
    }
    response_header_complete = true;

    std::string response_first_line;
    if (!HttpCommon::parseHeaders(response_header, response_first_line,
                                  http_response_headers)) {
        log() << "Bad HTTP headers: " << response_header;
        return false;
    }

    {
        // HTTP/1.1 200 OK
        unsigned int major, minor, status;
        int pos = sscanf(response_first_line.c_str(), "HTTP/%u.%u %u",
                         &major, &minor, &status);
        if (pos != 3 || !status)
            return false;

        set_http_version(major, minor);
        response_http_status = status;
    }

    for (auto p = http_response_headers.lower_bound("set-cookie");
         p!=http_response_headers.upper_bound("set-cookie"); ++p)
        owner_task->setCookie(p->second, current_uri);

    if (!websocket_rkey.empty()) {
        if (websocket_rkey == "CONNECT") {
            // Doing HTTP CONNECT through a proxy
            log() << "PROXY CONNECT status " << response_http_status;
            websocket_rkey.clear();
            proto_hostname.clear(); // No more proxy calls
            if (response_http_status < 200 || response_http_status >= 300)
                return false;
            response_header_complete = false;
            response_http_status = 0;
            current_request.clear();
            ws_get(current_uri);
            asyncSendData(current_request);
            return true;
        }

        auto p = http_response_headers.find("sec-websocket-accept");
        if (p == http_response_headers.end() || p->second != websocket_rkey) {
            err_log() << "wrong Sec-WebSocket-Accept";
            return false;
        }
        is_websocket = true;
        response_is_chunked = false;
        bytes_left_to_read = 0;
        return owner_task->websocketUpgrade(this);
    }

    try {
        auto p = http_response_headers.find("transfer-encoding");
        if (p != http_response_headers.end() && p->second == "chunked") {
            response_is_chunked = true;
            chunked_info = "\r\n";
            final_chunk = false;
            bytes_left_to_read = 0;
        } else {
            p = http_response_headers.find("content-length");
            if (p == http_response_headers.end()) {
                err_log() << "cannot find Content-Length";
                return false;
            }
            response_is_chunked = false;
            bytes_left_to_read = std::stoul(p->second);
        }
    } catch (std::exception &err) {
        err_log() << "Exception when parsing HTTP response: " << err.what();
        return false;
    }

    return owner_task->headerComplete(this);
}

PollState HttpClientConnection::writeData() {
    if (is_websocket)
        return wsWriteData();

    bytes_left_to_post -= owner_task->doPost(this, bytes_left_to_post);
    if (bytes_left_to_post)
        return PollState::WRITE;
    else
        return PollState::READ;
}

std::string HttpClientConnection::urlencode(const std::string &val) {
    std::ostringstream res;
    res.fill('0');
    res << std::hex;

    for (auto i=val.cbegin(); i!=val.cend(); ++i) {
        std::string::value_type c = *i;

        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            res << c;
        } else {
            res << '%' << std::setw(2)
                << std::uppercase << int(static_cast<unsigned char>(c))
                << std::nouppercase;
        }
    }

    return res.str();
}

void HttpClientConnection::
addUrlPars(std::string &url, const std::map<std::string, std::string> &pars) {
    auto p = pars.begin();
    if (p == pars.end())
        return;
    if (url.find('?') == std::string::npos)
        url += "?";
    else
        url += "&";
    ((url += p->first) += "=") += urlencode(p->second);
    while (++p != pars.end())
        (((url += "&") += p->first) += "=") += urlencode(p->second);
}

void HttpClientConnection::get(const std::string &url) {
    if (!idle()) {
        err_log() << "previous request not complete: " << current_request;
        return;
    }
    current_uri = url;
    current_request = "GET " + proto_hostname +
        url + " HTTP/1.1\r\nHost: " + real_hostname +
        "\r\n" + owner_task->httpHeaderLines(url) + "\r\n";
}

void HttpClientConnection::ws_get(const std::string &url) {
    if (!idle()) {
        err_log() << "previous request not complete: " << current_request;
        return;
    }
    if (!proto_hostname.empty()) {
        current_uri = url;
        websocket_rkey = "CONNECT";
        current_request = "CONNECT " + real_hostname + " HTTP/1.1\r\n"
            "Hostname: " + real_hostname + "\r\n\r\n";
        return;
    }
    // The first 24 chars will be filled with random base64 chars,
    // the rest is the websocket uuid.
    static char dst[] {
        "01234567890123456789====258EAFA5-E914-47DA-95CA-C5AB0DC85B11" };

    // 16 byte random data, convert to 24 base64 chars:
    int32_t rnd[4] = { rand(), rand(), rand(), rand() };
    const unsigned char *src = reinterpret_cast<const unsigned char *>(rnd);
    base64_encode(src, sizeof(rnd), dst);

    current_uri = url;
    current_request = "GET " + proto_hostname +
        url + " HTTP/1.1\r\nHost: " + real_hostname +
        "\r\nSec-WebSocket-Key: " + std::string(dst, 24) +
        "\r\nSec-WebSocket-Version: 13\r\nUpgrade: websocket"
        "\r\nConnection: Upgrade"
        "\r\n" + owner_task->httpHeaderLines(url) + "\r\n";
    static char rkey[] = "012345678901234567890123456=";
    static SHA1 sha1(rkey);
    sha1.update(dst);
    // The server's response is supposed to contain this string:
    websocket_rkey = std::string(rkey, 28);
}

void HttpClientConnection::post(const std::string &url,
                                const std::string &data) {
    if (!idle()) {
        err_log() << "previous request not complete: " << current_request;
        return;
    }
    current_uri = url;
    current_request = "POST " + proto_hostname + url + " HTTP/1.1\r\nHost: " +
        real_hostname + "\r\n" + owner_task->httpHeaderLines(url) +
        "Content-Length: " + std::to_string(data.size()) + "\r\n\r\n" + data;
}

void HttpClientConnection::post(const std::string &url, size_t len) {
    if (!idle()) {
        err_log() << "previous request not complete: " << current_request;
        return;
    }
    current_uri = url;
    current_request = "POST " + proto_hostname + url + " HTTP/1.1\r\nHost: " +
        real_hostname + "\r\n" + owner_task->httpHeaderLines(url) +
        "Content-Length: " + std::to_string(len) + "\r\n\r\n";
    bytes_left_to_post = len;
}

std::string HttpClientConnection::cacheLabel() {
    return owner_task->cacheLabel();
}

void HttpClientConnection::setOwner(Task *new_owner) {
    owner_task = dynamic_cast<HttpClientTask *>(new_owner);
    if (!owner_task)
        throw std::logic_error("expected HttClientTask");
    Socket::setOwner(new_owner);
}
