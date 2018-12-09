// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

// This class implements HTTP functionality common to server and client,
// mainly the websocket protocol.

#pragma once

#include "../framework/socketconnection.h"

class HttpConnection : public SocketConnection {
public:

    // 10 for HTTP/1.0, 11 for 1.1, 20 for 2.0 etc.
    unsigned int httpVersion() {
        return http_version;
    }

    void sendWsMessage(const std::string &msg);
    void sendWsBinary(const char *buf, size_t len);

    // Initiate sending a very large message. The owner task's sendWsData will
    // be called repetedly until all data has been sent.
    void startWsBinStream(size_t len) {
        startWsStream(len, true);
    }
    void startWsTxtStream(size_t len) {
        startWsStream(len, false);
    }
    void abortWsStream() {
        closeMe();
    }
    void sendWsClose(uint16_t code, std::string msg);
    void setOwner(Task *new_owner) override;

    // By default, incoming messages will be buffered and not delivered (through
    // wsBinMessage or wsTextMessage) until the complete message has arrived.
    // Call this from within the owner task's wsBinHeader or wsTextHeader
    // callback to get the response "streamed", i.e. the response will be
    // delivered in "chunks" to wsBinData/wsTextData as they arrive. Note that
    // the data may be masked, and you will have to unmask it yourself.
    void streamWsResponse() {
        stream_incoming = true;
    }
    // Returns pointer to 4 byte incoming mask,
    // or nullptr if the response isn't masked.
    const unsigned char *responseMask() const {
        if (incoming_is_masked)
            return incoming_mask;
        else
            return nullptr;
    }

    size_t wsIncomingBytesLeft() const {
        return bytes_to_receive;
    }
    size_t wsBytesReceived() const {
        return tot_to_receive-bytes_to_receive;
    }
    size_t wsOutgoingBytesLeft() const {
        return bytes_to_send;
    }
    size_t wsBytesSent() const {
        return tot_to_send-bytes_to_send;
    }
    bool isWebsocket() const {
        return is_websocket;
    }
protected:
    HttpConnection(const std::string &label, Task *owner,
                   const std::string &hostname, uint16_t port,
                   uint16_t iptype = 0, struct addrinfo *local_addr = nullptr) :
    SocketConnection(label, owner, hostname, port, iptype, local_addr) {
    }
    HttpConnection(const std::string &label, Task *owner, int fd,
                   const char *ip, uint16_t port) :
        SocketConnection(label, owner, fd, ip, port) {
    }

    void set_http_version(unsigned int major, unsigned int minor) {
        http_version = 10U*major + minor;
    }

    void send_ws_handshake(const std::string &key);
    void send_ws_bin_header(size_t len);
    void send_ws_txt_header(size_t len);
    void send_ws_pong();
    PollState incoming_ws_data(const char *buf, size_t len);
    PollState incoming_ws_header(const char *buf, size_t len);
    PollState wsReadData(const char *buf, size_t len);
    PollState wsWriteData();

    // Incoming buffer
    std::string buffer;

    // Everything below is websocket stuff:
    bool is_websocket = false;
private:
    void startWsStream(size_t len, bool is_binary = true);
    bool receiving_message = false;
    bool sending_message = false;
    bool output_is_binary = false;
    bool stream_incoming = false;
    bool incoming_is_masked = false;
    bool incoming_is_binary = false;
    size_t bytes_to_send, tot_to_send;
    size_t bytes_to_receive, tot_to_receive;
    unsigned char incoming_mask[4];
    unsigned int http_version = 11; // 11 for 1.1, 20 for 2.0 etc.
};
