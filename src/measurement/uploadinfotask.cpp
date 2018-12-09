#include "uploadinfotask.h"

UploadInfoTask::
UploadInfoTask(const std::string &ticket, const HttpHost &server,
               double duration, double max_time) :
    ProgressTask("uploadinfo", ticket, server, 1, 3, duration, max_time) {
}

double UploadInfoTask::start() {
    checkConnectionCount();
    return timeout_s();
}

void UploadInfoTask::newRequest(HttpClientConnection *conn) {
    conn->get("/ulinfo/1.txt?t=" + t());
}


bool UploadInfoTask::headerComplete(HttpClientConnection *conn) {
    conn->doStreamResponse();
    return true;
}

void UploadInfoTask::payload(HttpClientConnection *, char *buf, size_t len) {
    buffer.append(buf, len);
    while (true) {
        std::string::size_type pos = buffer.find("\r\n");
        if (pos == std::string::npos)
            return;
        std::istringstream line(buffer.substr(0, pos));
        uint64_t byte_count;
        double duration;
        if (line >> byte_count >> duration) {
            log() << "server upload info " << byte_count << ' ' << duration;
            notifyBytesAndDuration(byte_count, duration);
        } else if (pos) {
            err_log() << "bad server info line: " << buffer.substr(0, pos);
        }
        buffer.erase(0, pos+2);
    }
}
