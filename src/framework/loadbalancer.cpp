// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#include <signal.h>
#include "loadbalancer.h"
#include "socketreceiver.h"
#include "workerprocess.h"

LoadBalancer::LoadBalancer(const TaskConfig &tc) :
    Task("LoadBalancer"), my_config(tc) {
    try {
        tot_no_workers = std::stoul(my_config.value("workers"));
    } catch (...) {
        tot_no_workers = 0;
    }
    auto range = tc.cfg().equal_range("workercfg");
    for (auto p=range.first; p!=range.second; ++p)
        (worker_config += p->second) += "\n";
}

LoadBalancer::~LoadBalancer() {
    for (WorkerProcess *wp : worker_proc)
        if (wp)
            delete wp;
}

#ifdef USE_GNUTLS
bool LoadBalancer::tlsSetKey(ServerSocket *conn, const std::string &crt_path,
                             const std::string &key_path,
                             const std::string &password) {
    portMap[conn] = no_channels;
    worker_config += "channel" + std::to_string(no_channels) + " tls " +
        crt_path + " " + key_path + " " + password + "\n";
    ++no_channels;
    return true;
}
#endif

double LoadBalancer::start() {
    if (!tot_no_workers) {
        setError("Internal error, no workers");
        return 0.0;
    }

    worker_proc.resize(tot_no_workers);
    if (parseListen(my_config, "LoadBalancerSocket")) {
        for (size_t i=0; i<tot_no_workers; ++i)
            new_worker(i);
    } else {
        setResult("Failed, cannot start server");
    }
    return 0.0;
}

SocketConnection *LoadBalancer::newClient(int fd, const char *, uint16_t,
                                          ServerSocket *srv) {
    doPass(fd, nextWorker(), srv);
    rotateNextWorker();
    return nullptr;
}

void LoadBalancer::serverRemoved(ServerSocket *s) {
    log() << "Server removed socket " << s;
    if (SocketReceiver *conn = dynamic_cast<SocketReceiver *>(s)) {
        if (pid_t peer = conn->peerPid()) {
            removeWorker(peer);
            log() << "Kill process " << peer;
            kill(peer, SIGKILL);
        }
    } else {
        log() << "Not a receiver";
    }
}

void LoadBalancer::processFinished(int pid, int wstatus) {
    log() << "End of PID " << pid << ", status " << wstatus;
    removeWorker(pid);
}

void LoadBalancer::new_worker(size_t i) {
    if (worker_proc[i]) {
        delete worker_proc[i];
        worker_proc[i] = nullptr;
    }

    if (isFinished())
        return;

    std::string logfilename = my_config.value("workerlog");
    if (logfilename.empty()) {
        worker_proc[i] = createWorker(nullptr, no_channels,
                                      static_cast<unsigned int>(i));
    } else {
        // In logfilename, replace all %d with worker number (i.e. i),
        // adjusting to fixed width by filling with zeroes.
        auto len = std::to_string(tot_no_workers).size();
        auto wno = std::to_string(i);
        wno = std::string(len-wno.size(), '0') + wno;
        while (true) {
            auto pos = logfilename.find("%d");
            if (pos == std::string::npos)
                break;
            logfilename.replace(pos, 2, wno);
        }
        worker_proc[i] = createWorker(logfilename, no_channels,
                                      static_cast<unsigned int>(i));
    }
    if (worker_proc[i])
        log() << "Created worker " << i << ", pid: " << worker_proc[i]->pid();
}

void LoadBalancer::removeWorker(pid_t pid) {
    for (size_t i=0; i<worker_proc.size(); ++i)
        if (worker_proc[i] && worker_proc[i]->pid() == pid) {
            if (max_retries) {
                --max_retries;
            } else {
                setResult("Too many failures");
            }
            new_worker(i);
            break;
        }
}
