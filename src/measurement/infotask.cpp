// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#include "infotask.h"

void InfoTask::newRequest(HttpClientConnection *conn) {
    conn->ws_get("/minfo?t=" + t() + "&key=" + key);
}

bool InfoTask::wsTextMessage(HttpConnection *,
                             const std::string &msg) {
    log() << "MEASUREMENT INFO: " << msg;
    setMessage(msg);
    return true;
}

double InfoTask::timerEvent() {
    double time = elapsed();
    log() << "IT TimerEvent " << elapsed();
    if (time >= timeout_s()) {
        log() << "InfoTask timeout after " << time << " seconds";
        setResult("");
        return 0;
    }

    if (upload_deadline > 0.0) {
        log() << "waiting for server upload, t=" << time << ", max="
              << upload_deadline;
        if (time >= upload_deadline) {
            setMessage("server upload timeout");
            upload_deadline = -1.0;
        }
        return 0.1;
    } else if (info_deadline > 0.0) {
        log() << "waiting for measurement info, t=" << time << ", max="
              << info_deadline;
        if (time >= info_deadline) {
            setResult("");
            info_deadline = -1.0;
        }
        return 0.1;
    }

    return 1.0;
}
