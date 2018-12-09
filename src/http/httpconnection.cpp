// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "httpconnection.h"
#include "sha1.h"
#include "webservertask.h"

void HttpConnection::send_ws_handshake(const std::string &key) {
    static const std::string resp = "HTTP/1.1 101 Switching Protocols\r\n"
        "Connection: Upgrade\r\n"
        "Upgrade: websocket\r\n"
        "Sec-WebSocket-Accept: ";
    static thread_local char rkey[] = "012345678901234567890123456=";
    static thread_local SHA1 sha1(rkey);

    sha1.update((key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").c_str());

    asyncSendData(resp + rkey + "\r\n\r\n");
}

PollState HttpConnection::wsReadData(const char *buf, size_t len) {
    //log() << "wsReadData L=" << len << ", RCV: " << receiving_message;
    if (receiving_message) {
        if (!stream_incoming)
            return incoming_ws_data(buf, len);
        size_t count;
        if (len >= bytes_to_receive) {
            len -= bytes_to_receive;
            count = bytes_to_receive;
            bytes_to_receive = 0;
            receiving_message = false;
        } else {
            bytes_to_receive -= len;
            count = len;
            len = 0;
        }
        HttpTask *owner_task = dynamic_cast<HttpTask *>(owner());
        if (!owner_task)
            return PollState::CLOSE;
        bool ret;
        if (incoming_is_binary)
            ret = owner_task->wsBinData(this, buf, count);
        else
            ret = owner_task->wsTextData(this, buf, count);
        if (!ret)
            return PollState::CLOSE;
        if (!len)
            return (sending_message ? PollState::READ_WRITE :  PollState::READ);
        buf += count;
    }
    if (buffer.empty())
        return incoming_ws_header(buf, len);
    std::string tmp = buffer;
    tmp.append(buf, len);
    buffer.clear();
    return incoming_ws_header(tmp.data(), tmp.size());
}

PollState HttpConnection::incoming_ws_data(const char *buf, size_t len) {
    // Unmask, append to buffer (up to bytes_to_receive)
    size_t count;
    if (len >= bytes_to_receive) {
        count = bytes_to_receive;
        len -= bytes_to_receive;
        bytes_to_receive = 0;
    } else {
        count = len;
        bytes_to_receive -= len;
        len = 0;
    }
    if (incoming_is_masked) {
        const unsigned char *msg =
            reinterpret_cast<const unsigned char *>(buf);
        buf += count;
        while (count) {
            unsigned char c = *msg++ ^ incoming_mask[buffer.size()%4];
            buffer.push_back(static_cast<char>(c));
            --count;
        }
    } else {
        buffer.append(buf, count);
        buf += count;
    }
    if (!bytes_to_receive) {
        HttpTask *owner_task = dynamic_cast<HttpTask *>(owner());
        if (!owner_task)
            return PollState::CLOSE;
        bool ret;
        if (incoming_is_binary)
            ret = owner_task->wsBinMessage(this, buffer);
        else
            ret = owner_task->wsTextMessage(this, buffer);
        if (!ret)
            return PollState::CLOSE;
        buffer.clear();
        receiving_message = false;
    }
    if (len)
        return wsReadData(buf, len);
    return (sending_message ? PollState::READ_WRITE :  PollState::READ);
}

PollState HttpConnection::incoming_ws_header(const char *buf, size_t buffer_pos) {
    if (buffer_pos < 2) {
        buffer.append(buf, buffer_pos);
        return (sending_message ? PollState::READ_WRITE :  PollState::READ);
    }

    const unsigned char *msg =
        reinterpret_cast<const unsigned char *>(buf);
    size_t hdr_len;
    size_t msg_len = msg[1] & 0x7f;
    if (msg_len < 126) {
        hdr_len = 2;
    } else if (msg_len == 126) {
        // 2 bytes message length
        hdr_len = 4;
        if (buffer_pos >= hdr_len)
            msg_len = ntohs(*reinterpret_cast<const uint16_t *>(msg+2));
    } else {
        // msg_len == 127, 8 byte message length
        hdr_len = 10;
        /* Will not support message size > 4GB
           msg_len = (((uint64_t) ntohl(*(uint32_t *) (msg+2))) << 32) +
           ntohl(*(uint32_t *) (msg+6));
        */
        if (buffer_pos >= hdr_len)
            msg_len = ntohl(*reinterpret_cast<const uint32_t *>(msg+6));
    }

    if (buffer_pos < hdr_len) {
        buffer.append(buf, buffer_pos);
        return (sending_message ? PollState::READ_WRITE :  PollState::READ);
    }

    unsigned char opcode = msg[0] & 0xf;
    //log() << "OPCODE: " << (int)opcode << " LEN: " << msg_len << " HLEN: " << hdr_len << " <" << (int)msg[0] << "><" << (int)msg[1] << "><" << (int)msg[2] << "><" << (int)msg[3] << "> msg=" << (unsigned long) msg;
    //bool is_final = (msg[0] & 0x80);
    incoming_is_masked = msg[1] & 0x80;
    if (incoming_is_masked) {
        memcpy(incoming_mask, msg + hdr_len, 4);
        hdr_len += 4;
    }

    HttpTask *owner_task = dynamic_cast<HttpTask *>(owner());
    if (!owner_task)
        return PollState::CLOSE;

    switch (opcode) {
    case 1:
        stream_incoming = false;
        receiving_message = true;
        incoming_is_binary = false;
        bytes_to_receive = msg_len;
        tot_to_receive = msg_len;
        if (!owner_task->wsTextHeader(this, msg_len))
            return PollState::CLOSE;
        break;
    case 2:
        bytes_to_receive = msg_len;
        tot_to_receive = msg_len;
        stream_incoming = false;
        receiving_message = true;
        incoming_is_binary = true;
        if (!owner_task->wsBinHeader(this, msg_len))
            return PollState::CLOSE;
        break;
    case 8:
        // Closed by client
        log() << "ws closed by peer";
        return PollState::CLOSE;
    case 9:
        // Ping
        send_ws_pong();
        break;
    case 10:
        // Pong, ignore.
        break;
    default:
        err_log() << "Bad websocket opcode";
        return PollState::CLOSE;
    }
    buffer_pos -= hdr_len;
    if (buffer_pos)
        return wsReadData(buf+hdr_len, buffer_pos);
    return (sending_message ? PollState::READ_WRITE :  PollState::READ);
}

void HttpConnection::send_ws_bin_header(size_t len) {
    if (sending_message) {
        log() << "Internal error: sending message while streaming";
        closeMe();
        return;
    }
    unsigned char hdr[11];
    size_t pos = 0;
    hdr[pos++] = 0x80 | 0x02;  // Final fragment, type binary
    if (len > 65535) {
        hdr[pos++] = 127;
        uint32_t l = 0;
        memcpy(&hdr[pos], &l, 4);
        pos += 4;
        l = htonl(static_cast<uint32_t>(len));
        memcpy(&hdr[pos], &l, 4);
        pos += 4;
    } else {
        hdr[pos++] = 126;
        uint16_t l = htons(static_cast<uint16_t>(len));
        memcpy(&hdr[pos], &l, 2);
        pos += 2;
    }
    asyncSendData(reinterpret_cast<const char *>(hdr), pos);
}

void HttpConnection::send_ws_txt_header(size_t len) {
    if (sending_message) {
        log() << "Internal error: sending message while streaming";
        closeMe();
        return;
    }
    // TODO: support for masking the message.
    unsigned char hdr[11];
    unsigned int pos = 0;
    hdr[pos++] = 0x80 | 0x01;  // Final fragment, type text
    if (len < 126) {
        hdr[pos++] = static_cast<unsigned char>(len);
    } else if (len < 65536) {
        hdr[pos++] = 126;
        uint16_t l = htons(static_cast<uint16_t>(len));
        memcpy(&hdr[pos], &l, 2);
        pos += 2;
    } else {
        hdr[pos++] = 127;
        uint32_t l = 0;
        memcpy(&hdr[pos], &l, 4);
        pos += 4;
        l = htonl(static_cast<uint32_t>(len));
        memcpy(&hdr[pos], &l, 4);
        pos += 4;
    }
    asyncSendData(reinterpret_cast<char *>(hdr), pos);
}

void HttpConnection::sendWsMessage(const std::string &msg) {

    size_t len = msg.size();

    if (sending_message) {
        log() << "Internal error: sending message while streaming";
        closeMe();
        return;
    }
    // TODO: support for masking the message.
    unsigned char hdr[11];
    unsigned int pos = 0;
    hdr[pos++] = 0x80 | 0x01;  // Final fragment, type text
    if (len < 126) {
        hdr[pos++] = static_cast<unsigned char>(len);
    } else if (len < 65536) {
        hdr[pos++] = 126;
        uint16_t l = htons(static_cast<uint16_t>(len));
        memcpy(&hdr[pos], &l, 2);
        pos += 2;
    } else {
        hdr[pos++] = 127;
        uint32_t l = 0;
        memcpy(&hdr[pos], &l, 4);
        pos += 4;
        l = htonl(static_cast<uint32_t>(len));
        memcpy(&hdr[pos], &l, 4);
        pos += 4;
    }

    asyncSendData(std::string(reinterpret_cast<char *>(hdr), pos) + msg);
}

void HttpConnection::sendWsBinary(const char *buf, size_t len) {
    send_ws_bin_header(len);
    asyncSendData(buf, len);
}

void HttpConnection::sendWsClose(uint16_t code, std::string msg) {

    if (msg.size() > 124)
        msg.resize(124);  // Will only allow a very short message.

    unsigned char hdr[4];

    hdr[0] = 0x80 | 0x08;  // Final fragment, opcode CLOSE
    // Will send 2 byte error code, then the msg:
    hdr[1] = static_cast<unsigned char>(2 + msg.size());

    // The error code:
    uint16_t l = htons(code + 2);
    memcpy(hdr+2, &l, 2);

    asyncSendData(std::string(reinterpret_cast<char *>(hdr), 4) + msg);
    closeMe();
}

void HttpConnection::send_ws_pong() {
    unsigned char hdr[2];

    hdr[0] = 0x80 | 0x0a;  // Final fragment, opcode PONG
    hdr[1] = 0;  // No payload, length 0.

    asyncSendData(reinterpret_cast<char *>(hdr), 2);
}

void HttpConnection::startWsStream(size_t len, bool is_binary) {
    // TODO: also text
    log() << "startWsStream " << len;
    if (is_binary)
        send_ws_bin_header(len);
    else
        send_ws_txt_header(len);
    bytes_to_send = len;
    tot_to_send = len;
    sending_message = true;
    output_is_binary = is_binary;
    setWantToSend();
}

PollState HttpConnection::wsWriteData() {
    if (!sending_message)
        return PollState::READ;
    HttpTask *owner_task = dynamic_cast<HttpTask *>(owner());
    if (!owner_task)
        return PollState::CLOSE;
    size_t sent = owner_task->sendWsData(this);
    if (sent > bytes_to_send)
        return PollState::CLOSE;
    bytes_to_send -= sent;
    if (bytes_to_send)
        return PollState::READ_WRITE;
    sending_message = false;
    return PollState::READ;
}

void HttpConnection::setOwner(Task *) {
}
