// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#include <sstream>
#include "rpingtask.h"

void RpingTask::newRequest(HttpClientConnection *conn) {
    conn->ws_get("/ws?t=" + t());
}

bool RpingTask::wsTextMessage(HttpConnection *conn,
                             const std::string &msg) {
    if (!max_roundtrips) {
        setResult("");
        return false;
    }
    --max_roundtrips;

    std::istringstream line(msg);
    std::string cmd, challenge;
    double val;
    line >> cmd;
    if (sent_challenge) {
        line >> val;
        if (cmd == "latency_result") {
            setResult(fValue(val));
            return false;
        } else if (line && val > 0.0) {
            samples.push_back(val);
        }
    }

    if (cmd != "challenge") {
        err_log() << "unknown message: " << msg;
        return false;
    }
    log() << "Received " << msg;

    line >> challenge;
    if (!line || challenge.size() > 10) {
        err_log() << "Bad rping challenge";
        return false;
    }

    if (samples.size() >= 12) {
        conn->sendWsMessage("rping end");
        log() << "Sending rping end";
        return true;
    }

    conn->sendWsMessage("rping " + challenge);
    log() << "Sending rping " << challenge;
    sent_challenge = true;
    return true;
}

bool RpingTask::websocketUpgrade(HttpClientConnection *conn) {
    conn->sendWsMessage("rping start");
    log() << "Sending rping start";
    return true;
}
