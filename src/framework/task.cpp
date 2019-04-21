// Copyright (c) 2019 Internetstiftelsen
// Written by Göran Andersson <initgoran@gmail.com>

#include "task.h"
#include "socketconnection.h"
#include "serversocket.h"

#ifdef USE_THREADS
thread_local
#endif
EventLoop *Task::supervisor = nullptr;

Task::Task(const std::string &task_name) :
    Logger(task_name) {
    log() << "Task " << task_name << " created";
}

void Task::setResult(const std::string &res) {
    if (is_finished) {
        dbg_log() << "result already set, ignoring " << res;
    } else {
        dbg_log() << "setResult " << res;
        is_finished = true;
        the_result = res;
        if (has_started)
            supervisor->notifyTaskFinished(this);
    }
}

void Task::setMessage(const std::string &msg) {
    the_message = msg;
    supervisor->notifyTaskMessage(this);
}

PollState Task::connectionReady(SocketConnection * /* conn */) {
    log() << "connectionReady not implemented";
    return PollState::CLOSE;
}

PollState Task::msgFromConnection(SocketConnection * /* conn */,
                            const std::string & /* msg */) {
    log() << "msgFromConnection not implemented";
    return PollState::CLOSE;
}

bool Task::addConnection(SocketConnection *conn) {
    if (terminated()) {
        if (conn)
            delete conn;
        return false;
    }
    return supervisor->addConnection(conn);
}

bool Task::addConnected(SocketConnection *conn) {
    if (terminated()) {
        if (conn)
            delete conn;
        return false;
    }
    return supervisor->addConnected(conn);
}

bool Task::adoptConnection(Socket *conn) {
    conn->setOwner(this);
    return true;
}

std::set<Socket *> Task::getMyConnections() const {
    return supervisor->findConnByTask(this);
}

void Task::wakeUp() {
    supervisor->wakeUpTask(this);
}

Task::~Task() {
    supervisor->taskDeleted(this);
}

bool Task::addServer(ServerSocket *conn) {
    if (terminated()) {
        if (conn)
            delete conn;
        return false;
    }
    return supervisor->addServer(conn);
}

int Task::runProcess(const char *const argv[]) {
    return supervisor->externalCommand(this, argv);
}

void Task::processFinished(int pid, int wstatus) {
    log() << "Process " << pid << " finished, status " << wstatus;
}

bool Task::parseListen(const TaskConfig &tc, const std::string &log_label) {
    auto to = tc.cfg().upper_bound("listen");
    for (auto p=tc.cfg().lower_bound("listen"); p!=to; ++p) {
        std::istringstream s(p->second);
        uint16_t port;
        std::string ip;
        s >> port;
        if (!s) {
            err_log() << "Bad configuration directive: listen " << p->second;
            setError("bad configuration directive");
            return false;
        }
        s >> ip;
        bool tls;
        if (ip.empty()) {
            tls = false;
        } else if (ip == "tls") {
            ip.clear();
            tls = true;
        } else {
            std::string tmp;
            s >> tmp;
            tls = (tmp == "tls");
        }
        auto sock = new ServerSocket(log_label, this, port, ip);
#ifdef USE_GNUTLS
        if (tls) {
            std::string cert, key, password;
            s >> cert >> key >> password;
            if (key.empty() || !tlsSetKey(sock, cert, key, password)) {
                err_log() << "Bad configuration: " << p->second;
                setError("cannot use TLS certificate");
                return false;
            }
            log() << "Port " << port << " enable TLS";
        }
#else
        if (tls) {
            err_log() << "cannot enable TLS, will not listen on port " << port;
            return false;
        }
#endif
        if (!addServer(sock)) {
            setError("cannot listen");
            return false;
        }
    }
    return true;
}
