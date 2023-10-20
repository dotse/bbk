// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

// This class implements a network engine, managing sockets.
// It is only used by the EventLoop class.

#pragma once

#include <thread>
#include <map>
#include <vector>
#include <string>

#include "logger.h"

#ifdef _WIN32
#include <winsock2.h>
#endif

class Socket;
class SocketConnection;
class ServerSocket;
class Task;

#ifdef USE_GNUTLS
#include <gnutls/gnutls.h>
#endif

/// \brief
/// The network engine.
///
/// Cannot be used directly. Will be managed by the EventLoop class.
class Engine : public Logger {
public:
    bool addClient(SocketConnection *conn);
    void addConnected(SocketConnection *conn);
    bool addServer(ServerSocket *conn);

    Engine(std::string label);

    /// Will kill all remaining connections.
    ~Engine();

    /// Close all connections, giving them at most
    /// max_time_ms milliseconds to finish what's
    /// already written to them.
    void terminate(unsigned int max_time_ms);

    /// Call this in child process to close all redundant sockets after fork.
    void childProcessCloseSockets();

    /// \brief
    /// Run the "event loop" for at most `max_time` seconds.
    ///
    /// Will return when all connections have been closed,
    /// when a fatal error has occurred, or when max_time has passed.
    /// Return value is false on fatal error, otherwise true.
    /// You should call this function repetedly until you're done.
    ///
    /// Don't set max_time > 2000 on 32-bit platforms.
    bool run(double max_time);

    /// Remove all connections owned by the task.
    void deleteConnByTask(const Task *task);

    /// Call this to make the Engine::run method return prematurely.
    static void yield() {
        yield_called = true;
    }

    /// \brief
    /// Call this to enter a recovery mode if no more file descriptors
    /// could be created.
    static void notifyOutOfFds() {
        // TODO: thread safe
        max_open_fd_reached = true;
    }
    std::set<Socket *> findSockByTask(const Task *t) const;

    /// Wake up all idle connections belonging to t:
    void wakeUpByTask(Task *t);

    /// Wake up connection s if it is idle, return false otherwise.
    bool wakeUpConnection(SocketConnection *s);
    void cancelConnection(SocketConnection *s);

    /// Return true if connection still exists.
    /// *Note!* We cannot _use_ conn if it has been deleted!
    bool connActive(const Socket *conn) const {
        for (auto it : connectionStore)
            if (it.second == conn)
                return true;
        return false;
    }

    /// Call this to make the Engine::run method return earlier.
    void resetDeadline(const TimePoint &t) {
        if (deadline > t)
            deadline = t;
    }

#ifdef USE_GNUTLS
    /// Set path to file containing chain of trust for SSL certificate.
    bool setCABundle(const std::string &path);

    /// Use SSL certificate for a listening socket.
    bool tlsSetKey(ServerSocket *conn, const std::string &crt_path,
                   const std::string &key_path, const std::string &password);
#endif

private:

    TimePoint deadline;
#ifdef USE_THREADS
    thread_local
#endif
    static volatile bool yield_called;
    static bool max_open_fd_reached;
    void handleMaxOpenFdReached();
    void killConnection(int fd);
    void handleIncoming(ServerSocket *server);
    // Check all connections, remove closed, reclaim keepalive,
    // return false if none was removed:
    bool reclaimConnections();

    // Prepare for select, return largest fd or -1:
    int setFds(fd_set &r, fd_set &w, fd_set &e);
    // Check error from select, return false if fatal:
    bool fatalSelectError();
    // Take care of all network events:
    void doFds(const fd_set &r, const fd_set &w, const fd_set &e, int max);

    // Map socket number to Socket object:
    std::map<int, Socket *> connectionStore;

    // Cache of open connections that may be reused.
    // Value is socket number. Key is some kind of label,
    // chosen by the tasks owning the connections,
    // that should identify the type of connection.
    std::multimap<std::string, int> keepaliveCache;
#ifdef USE_GNUTLS
    std::map<int, gnutls_session_t> tls_session_cache;
    std::string ca_bundle;
    std::map<std::string, unsigned int> tls_crt_map;
    // Index 0 is used for outgoing connection, containing trust store.
    // At index > 0 certificates for server sockets are stored.
    std::vector<gnutls_certificate_credentials_t> x509_cred;
    gnutls_priority_t priority_cache;
#endif
};
