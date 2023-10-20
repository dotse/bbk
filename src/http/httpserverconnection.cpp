// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#ifdef _WIN32
#define NOMINMAX
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <fcntl.h>

#ifdef _WIN32
#include <time.h>
#include <windows.h>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#endif
#include <sys/stat.h>

#include <sstream>
#include <algorithm>
#include "sha1.h"

#include "httpserverconnection.h"
#include "http_common.h"
#include "webservertask.h"

HttpServerConnection::
HttpServerConnection(const std::string &label, WebServerTask *task,
                     int fd, const char *ip, uint16_t port) :
    HttpConnection(label, task, fd, ip, port),
    owner_task(task) {
}

void HttpServerConnection::setOwner(Task *new_owner) {
    owner_task = dynamic_cast<WebServerTask *>(new_owner);
    if (!owner_task)
        throw std::logic_error("expected WebServerTask");
    Socket::setOwner(new_owner);
}

PollState HttpServerConnection::got_post_data(const char *buf, size_t len) {
    //log() << "got_post_data " << len << " bytes, " << remaining_post_data
    // << " left\n" << std::string(buf, len);
    size_t length = std::min(remaining_post_data, len);
    remaining_post_data -= length;
    if (remaining_post_data) {
        if (!buffer_post_data)
            if (!owner_task->partialPostData(this, buf, length))
                return PollState::CLOSE;
    } else {
        if (buffer_post_data)
            state = owner_task->lastPostData(this, buffer.c_str(),
                                             buffer.size());
        else
            state = owner_task->lastPostData(this, buf, len);
        if (len > length) {
            err_log() << "too much post data sent";
            return PollState::CLOSE;
        }
        if (state == HttpState::SENDING_RESPONSE)
            return PollState::WRITE;
        else if (state == HttpState::CLOSE)
            return PollState::CLOSE;
    }
    return PollState::READ;
}

#ifdef USE_WEBROOT
bool HttpServerConnection::prepareFileResponse(const std::string &url) {

    if (url.find("../") != std::string::npos)
        return false;

    std::string webroot = owner_task->webRoot();
    if (webroot.empty())
        return false;

    static_file_fd = open( (webroot + url).c_str(), O_RDONLY );
    if (static_file_fd < 0)
        return false;

    struct stat sbuf;
    if (fstat(static_file_fd, &sbuf) < 0 || !S_ISREG(sbuf.st_mode)) {
        close(static_file_fd);
        return false;
    }

    remaining_bytes = static_cast<size_t>(sbuf.st_size);
    sending_static_file = true;
    return true;
}

HttpState HttpServerConnection::sendHttpFileResponse(const std::string &hdrs,
                                                     const std::string &mime) {
    if (!sending_static_file || state != HttpState::WAITING_FOR_REQUEST) {
        err_log() << "internal error, prepareFileResponse not called";
        return HttpState::CLOSE;
    }

    sendHttpResponseHeader(hdrs, mime, remaining_bytes);

    if (remaining_bytes)
        remaining_bytes -= sendFileData(static_file_fd, remaining_bytes);

    if (remaining_bytes)
        return HttpState::SENDING_RESPONSE;
    else
        return HttpState::WAITING_FOR_REQUEST;
}
#endif

PollState HttpServerConnection::writeData() {
    if (is_websocket)
        return wsWriteData();
    if (state == HttpState::SENDING_RESPONSE) {
        // TODO: don't give conn to the owner, too much that can go wrong.

        size_t sent;
#ifdef USE_WEBROOT
        if (sending_static_file)
            sent = sendFileData(static_file_fd, remaining_bytes);
        else
#endif
            sent = owner_task->sendResponseData(this, remaining_bytes);

        remaining_bytes -= sent;
        if (remaining_bytes)
            return PollState::WRITE;
#ifdef USE_WEBROOT
        if (sending_static_file) {
            close(static_file_fd);
            sending_static_file = false;
        }
#endif
        state = HttpState::WAITING_FOR_REQUEST;
        return PollState::READ;
    }
    err_log() << "HttpServerConnection::writeData() called";
    return PollState::CLOSE;
}

PollState HttpServerConnection::readData(char *buf, size_t len) {
    if (is_websocket)
        return wsReadData(buf, len);
    if (state == HttpState::READING_POST_DATA && !buffer_post_data)
        return got_post_data(buf, len);
    buffer.append(buf, len);
    return check_buffer();
}

PollState HttpServerConnection::check_buffer() {
    if (state == HttpState::WAITING_FOR_REQUEST) {
        size_t end_of_request = buffer.find("\r\n\r\n");
        if (end_of_request == std::string::npos) {
            if (buffer.size() > 20000) {
                log() << "error: no end of request";
                return PollState::CLOSE;
            }
            return PollState::READ;
        }
        //log() << "Got request: " << buffer;
        current_request = buffer.substr(0, end_of_request+strlen("\r\n\r\n"));
        buffer.erase(0, current_request.size());
        if (!parse_request()) {
            err_log() << "bad request";
            return PollState::CLOSE;
        }
        if (current_method == "GET") {
            auto p = http_request_headers.find("sec-websocket-key");
            if (p == http_request_headers.end()) {
                state = owner_task->newGetRequest(this, current_uri);
            } else {
                if (!owner_task->newWsRequest(this, current_uri))
                    return PollState::CLOSE;
                state = HttpState::WEBSOCKET;
                is_websocket = true;
                send_ws_handshake(p->second);
                if (notify_after_ws_handshake)
                    owner_task->wsHandshakeFinished(this, current_uri);
            }
            if (buffer.size()) {
                log() << "unexpected data after GET request";
                return PollState::CLOSE;
            }
        } else if (current_method == "POST") {
            try {
                auto p = http_request_headers.find("content-length");
                if (p == http_request_headers.end()) {
                    err_log() << "cannot find Content-Length";
                    // Just assume a very large post:
                    remaining_post_data = std::string::npos;
                } else {
                    remaining_post_data = std::stoul(p->second);
                }
            } catch (std::exception &) {
                err_log() << "cannot parse Content-Length";
                return PollState::CLOSE;
            }
            buffer_post_data = false;
            state = owner_task->newPostRequest(this, current_uri);
        } else if (current_method == "OPTIONS") {
            state = owner_task->preflightRequest(this, current_uri);
        } else {
            return PollState::CLOSE;
        }
    }
    if (state == HttpState::CLOSE)
        return PollState::CLOSE;
    if (state == HttpState::SENDING_RESPONSE)
        return PollState::WRITE;
    if (buffer.empty())
        return PollState::READ;
    if (state == HttpState::READING_POST_DATA) {
        PollState ret = got_post_data(buffer.c_str(), buffer.size());
        if (!buffer_post_data || !remaining_post_data)
            buffer.clear();
        return ret;
    }
    return check_buffer();
}

namespace {
    inline int hexval(std::string::value_type c) {
        if (c >= '0' && c <= '9')
            return c-'0';
        else if (c >= 'A' && c <= 'F')
            return (c - 'A' + 10);
        else if (c >= 'a' && c <= 'f')
            return (c - 'a' + 10);
        else
            return -1000;
    }
}

void HttpServerConnection::get_all_parameters(const char *qpos,
                                              std::multimap<std::string,
                                              std::string> &pars) {
    const char *ppos = qpos; // Start of parameter name
    while (*qpos) {
        if (*qpos == '&') {
            // Epmty value:
            pars.insert(std::make_pair(std::string(ppos, qpos), std::string()));
            ++qpos;
            ppos = qpos;
        } else if (*qpos == '=') {
            // Urldecode value:
            std::string par_name = std::string(ppos, qpos);
            ++qpos; // Start of value
            std::ostringstream res;
            while (char c = *qpos) {
                if (c == '&') {
                    ++qpos;
                    break;
                } else if (c == '+') {
                    ++qpos;
                    c = ' ';
                } else if (c == '%') {
                    // Should be two hex digits after %,
                    // but if not, ignore errors and just keep the %
                    if (*++qpos) {
                        char c1 = *qpos, c2;
                        if (*++qpos) {
                            c2 = *qpos;
                            if (c2) {
                                ++qpos;
                                int n = hexval(c1)*16+hexval(c2);
                                if (n >= 0)
                                    c = static_cast<char>(n);
                            }
                        }
                    }
                } else {
                    ++qpos;
                }
                res << c;
            }
            pars.insert(std::make_pair(par_name, res.str()));
            ppos = qpos;
        } else {
            ++qpos;
        }
    }
}

bool HttpServerConnection::parse_request() {
    std::string first_line;
    http_request_headers.clear();
    if (!HttpCommon::parseHeaders(current_request, first_line,
                                  http_request_headers))
        return false;
    size_t eom = first_line.find(' ');
    if (eom == std::string::npos)
        return false;
    size_t eor = first_line.find(' ', eom+1);
    if (eor == std::string::npos)
        return false;
    current_method = first_line.substr(0, eom);
    current_uri = first_line.substr(eom+1, eor-eom-1);
    {
        // HTTP/1.1
        unsigned int major, minor;
        int pos = sscanf(first_line.substr(eor+1).c_str(), "HTTP/%u.%u",
                         &major, &minor);
        if (pos != 2)
            return false;

        set_http_version(major, minor);
    }
    current_query_pars.clear();
    size_t qpos = current_uri.find("?");
    if (qpos != std::string::npos) {
        current_query_string = current_uri.substr(qpos+1);
        current_uri.resize(qpos);
        get_all_parameters(current_query_string.c_str(), current_query_pars);
    } else {
        current_query_string.clear();
    }

    // uri might be given as http://some.domain/blah
    if (current_uri.substr(0, 4) == "http") {
        if (current_uri.substr(0, 7) == "http://")
            qpos = current_uri.find('/', 7);
        else if (current_uri.substr(0, 8) == "https://")
            qpos = current_uri.find('/', 8);
        else
            return true;
        if (qpos == std::string::npos)
            current_uri = "/";
        else
            current_uri.erase(0, qpos);
    }
    return true;
}

size_t HttpServerConnection::
sendHttpResponse(const std::string &headers,
                 const std::string &mime,
                 const std::string &contents) {
    std::ostringstream response;
    response << headers << "Content-Length: " << contents.size()
             << "\r\nContent-Type: " << mime << "\r\n\r\n" << contents;
    size_t len = response.str().size();
    asyncSendData(response.str());
    return len;
}

size_t HttpServerConnection::
sendHttpResponseHeader(const std::string &headers,
                       const std::string &mime,
                       size_t content_length) {
    std::ostringstream response;
    response << headers << "Content-Length: " << content_length
             << "\r\nContent-Type: " << mime << "\r\n\r\n";
    size_t len = response.str().size();
    asyncSendData(response.str());
    remaining_bytes = content_length;
    return len;
}

size_t HttpServerConnection::
sendChunkedResponseHeader(const std::string &headers,
                          const std::string &mime) {
    sending_chunked_response = true;
    std::ostringstream response;
    response << headers << "Transfer-Encoding: chunked"
             << "\r\nContent-Type: " << mime << "\r\n\r\n";
    size_t len = response.str().size();
    asyncSendData(response.str());
    return len;
}

size_t HttpServerConnection::sendChunk(const std::string &content) {
    if (!sending_chunked_response) {
        err_log("response encoding is not chunked");
        return 0;
    }
    std::ostringstream chunk;
    chunk << std::hex << content.size() << "\r\n" << content << "\r\n";
    asyncSendData(chunk.str());
    return chunk.str().size();
}

size_t HttpServerConnection::chunkedResponseComplete() {
    if (!sending_chunked_response) {
        err_log("response encoding is not chunked");
        return 0;
    }
    std::string last_chunk = "0\r\n\r\n";
    sending_chunked_response = false;
    asyncSendData(last_chunk);
    return last_chunk.size();
}

bool HttpServerConnection::hasQueryPar(const std::string &name) const {
    return (current_query_pars.find(name) != current_query_pars.end());
}

std::string HttpServerConnection::getQueryVal(const std::string &name) const {
    auto p = current_query_pars.find(name);
    if (p != current_query_pars.end())
        return p->second;
    return std::string();
}

std::string HttpServerConnection::getHeaderVal(const std::string &name) const {
    auto p = http_request_headers.find(name);
    if (p != http_request_headers.end())
        return p->second;
    return std::string();
}

std::map<std::string, std::string>
HttpServerConnection::currentRequestCookies() const {
    std::map<std::string, std::string> c;
    auto r = getHeaderVals("cookie");
    for (auto p=r.first; p!=r.second; ++p) {
        std::string s = p->second;
        while (!s.empty()) {
            auto pos1 = s.find('=');
            if (pos1 == std::string::npos)
                break;
            auto pos2 = s.find(';');
            c[s.substr(0, pos1)] = s.substr(pos1+1, pos2-pos1-1);
            if (pos2 == std::string::npos)
                break;
            s.erase(0, s.find_first_not_of(" \t\r", pos2+1));
        }
    }
    return c;
}

std::string HttpServerConnection::cookieVal(const std::string &name) const {
    if (name.find_first_of(" =;,\t") != std::string::npos)
        return "";
    auto r = getHeaderVals("cookie");
    for (auto p=r.first; p!=r.second; ++p) {
        auto pos = (" " + p->second).find(" " + name + "=");
        if (pos != std::string::npos) {
            auto first = p->second.find('=', pos);
            auto last = p->second.find(';', pos);
            if (last > first) {
                return p->second.substr(first+1, last-first-1);
            }
        }
    }
    return "";
}

HttpServerConnection::~HttpServerConnection() {
    dbg_log() << "client connection closed";
#ifdef USE_WEBROOT
    if (sending_static_file)
        close(static_file_fd);
#endif
}
