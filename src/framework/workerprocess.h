// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <vector>

class SocketReceiver;

/// \brief
/// Used by LoadBalancer to manage child processes.
///
/// One or more SocketReceiver objects may be used to pass sockets
/// (and/or messages) between master and child processes.
///
/// Each SocketReceiver will be called _a channel_, and they will be
/// referenced using integers starting with 0.
///
/// The point of using channels is to pass different types of connections
/// (e.g. with or without SSL encryption) on different channels.
class WorkerProcess {
public:
    /// Create worker to run in newly forked process `pid`.
    WorkerProcess(pid_t pid, std::vector<SocketReceiver *> &receivers) :
        worker_pid(pid),
        channels(receivers) {
    }

    ~WorkerProcess() {
        for (auto &conn : channels)
            conn->peerDead();
    }

    /// Return the PID of the worker process.
    pid_t pid() const {
        return worker_pid;
    }

    /// Return a channel.
    SocketReceiver *channel(unsigned int n=0) const {
        return channels.at(n);
    }

    /// Return number of channels.
    size_t noChannels() const {
        return channels.size();
    }

private:
    pid_t worker_pid;
    std::vector<SocketReceiver *> channels;
};
