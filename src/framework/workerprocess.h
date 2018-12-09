// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <vector>

class SocketReceiver;

class WorkerProcess {
public:
    WorkerProcess(pid_t pid, std::vector<SocketReceiver *> &receivers) :
        worker_pid(pid),
        channels(receivers) {
    }

    ~WorkerProcess() {
        for (auto &conn : channels)
            conn->peerDead();
    }

    pid_t pid() const {
        return worker_pid;
    }

    SocketReceiver *channel(unsigned int n=0) const {
        return channels.at(n);
    }

    size_t noChannels() const {
        return channels.size();
    }

private:
    pid_t worker_pid;
    std::vector<SocketReceiver *> channels;
};
