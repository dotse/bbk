// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#include <string.h>
#include <sys/types.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netdb.h>
#endif

#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <sys/select.h>
#endif

#include <set>
#include <stdexcept>

#include "engine.h"
#include "socketconnection.h"
#include "serversocket.h"
#ifndef _WIN32
#include <arpa/inet.h>
#endif

Engine::Engine(std::string label) :
    Logger(label) {
#ifdef USE_GNUTLS
    x509_cred.resize(1);
    if (gnutls_global_init() < 0 ||
        gnutls_certificate_allocate_credentials(&x509_cred[0]) < 0 ||
        gnutls_priority_init(&priority_cache,
                             "PERFORMANCE:%SERVER_PRECEDENCE",
                             nullptr) < 0)
        throw std::runtime_error("cannot enable TLS");
#if GNUTLS_VERSION_NUMBER >= 0x030506
        /* only available since GnuTLS 3.5.6, on previous versions see
         * gnutls_certificate_set_dh_params(). */
        gnutls_certificate_set_known_dh_params(x509_cred[0], GNUTLS_SEC_PARAM_MEDIUM);
#endif
#endif
}

Engine::~Engine() {

    terminate(0);
    Socket::clearCache();

#ifdef USE_GNUTLS
    for (auto &cred : x509_cred)
        gnutls_certificate_free_credentials(cred);
    gnutls_priority_deinit(priority_cache);
    gnutls_global_deinit();
#endif

}

#ifdef USE_GNUTLS

bool Engine::setCABundle(const std::string &path) {
    if (path.empty())
        return true;
    ca_bundle = path;
    if (gnutls_certificate_set_x509_trust_file(x509_cred[0], ca_bundle.c_str(),
                                               GNUTLS_X509_FMT_PEM) < 0)
        return false;
    return true;
}

bool Engine::tlsSetKey(ServerSocket *conn, const std::string &crt_path,
                       const std::string &key_path,
                       const std::string &password) {
    if (key_path.empty() && crt_path.empty())
        return 0;

    unsigned int index;
    auto it = tls_crt_map.find(crt_path);
    if (it != tls_crt_map.end()) {
        index = it->second;
    } else {
        index = static_cast<unsigned int>(x509_cred.size());
        x509_cred.resize(index+1);
        if (gnutls_certificate_allocate_credentials(&x509_cred[index]) < 0) {
            x509_cred.resize(index);
            return false;
        }
        int ret = gnutls_certificate_set_x509_key_file2(x509_cred[index],
                                                        crt_path.c_str(),
                                                        key_path.c_str(),
                                                        GNUTLS_X509_FMT_PEM,
                                                        password.c_str(), 0);
        if (ret < 0) {
            err_log() << "Cannot add TLS server certificate: "
                      << gnutls_strerror(ret);
            return false;
        }
        tls_crt_map[crt_path] = index;
    }
    conn->tlsSetKey(index);
    return true;
}

#endif

// Hand the SocketConnection object over to us.
// Will delete the object and return false on immediate failure.
// If the object already has a socket, i.e. conn->socket()>=0,
// it will be used.
// Otherwise we will listen on the given port number and
// local ip number (port 0 means any available port,
// leave ip_number empty to listen on all local ip numbers).
bool Engine::addServer(ServerSocket *conn) {
    if (!conn)
        return false;
    if (conn->socket()<0 && !conn->createServerSocket()) {
        err_log() << "cannot add ServerSocket "
              << conn->hostname() << ':' << conn->port();
        delete conn;
        return false;
    }
    dbg_log() << "addServer fd " << conn->socket();
    connectionStore[conn->socket()] = conn;
    return true;
}

// TODO: closedByPeer, connectionFailed

// Hand the SocketConnection object over to us.
// Will delete the object and return false on immediate failure.
// Otherwise return true; then either connected or connectionFailed
// will be called on conn.
bool Engine::addClient(SocketConnection *conn) {
    if (!conn)
        return false;

#ifndef _WIN32
    if (conn->socket() >= 0) {
        if (connectionStore.find(conn->socket()) != connectionStore.end()) {
            err_log() << "Socket " << conn->socket() << " host "
                      << conn->hostname() << " already exists";
            return false;
        }
        if (conn->hostname() != "UnixDomain" || !conn->socket()) {
            err_log() << "Socket " << conn->socket() << " host "
                      << conn->hostname() << " not accepted";
            return false;
        }

        // Unix Domain socket, socket already created and connected
        addConnected(conn);
        return true;
    }
#endif
    std::string label = conn->cacheLabel();
    auto p = keepaliveCache.find(label);
    if (!label.empty() &&
        p != keepaliveCache.end()) {
        log() << "Using cached socket " << p->second << " label " << label;
        conn->setSocket(p->second);
#ifdef USE_GNUTLS
        auto it = tls_session_cache.find(p->second);
        if (it != tls_session_cache.end()) {
            conn->insert_cached_session(it->second);
            tls_session_cache.erase(it);
        }
#endif
        keepaliveCache.erase(p);
        //log() << "Cache size is " << keepaliveCache.size();
        conn->setState(conn->connected());
    } else if (!conn->asyncConnect()) {
        delete conn;
        return false;
    }

    connectionStore[conn->socket()] = conn;
    return true;
}

void Engine::childProcessCloseSockets() {
    for (auto p : connectionStore)
        Socket::closeSocket(p.first);
    for (auto p : keepaliveCache)
        Socket::closeSocket(p.second);
}

void Engine::terminate(unsigned int ) {
    while (!connectionStore.empty())
        killConnection(connectionStore.cbegin()->first);
    for (auto p : keepaliveCache)
        Socket::closeSocket(p.second);
#ifdef USE_GNUTLS
    for (auto p : tls_session_cache)
        gnutls_deinit(p.second);
#endif
}

bool Engine::reclaimConnections() {
    std::set<int> fds_to_remove;
    for (auto p : connectionStore)
        switch (p.second->state()) {
        case PollState::CLOSE:
        case PollState::KILL:
        case PollState::KEEPALIVE:
            fds_to_remove.insert(p.first);
            break;
        case PollState::NONE:
        case PollState::READ_BLOCKED:
#ifdef USE_GNUTLS
        case PollState::TLS_HANDSHAKE:
#endif
        case PollState::CONNECTING:
        case PollState::WRITE:
        case PollState::READ_WRITE:
        case PollState::READ:
            break;
        }
    if (fds_to_remove.empty())
        return false;
    for (auto fd : fds_to_remove)
        killConnection(fd);
    return true;
}

int Engine::setFds(fd_set &r, fd_set &w, fd_set &e) {
    int max = -1;
    FD_ZERO(&r);
    FD_ZERO(&w);
    FD_ZERO(&e);

    // Check what to do with each connection.
    // Note! We cannot do anything non-trivial while looping
    // over connectionStore since elements might be added or
    // removed, invalidating iterators.
    for (auto p : connectionStore) {
        int fd = p.first;
        Socket *c = p.second;
        switch (c->state()) {
        case PollState::CLOSE:
        case PollState::KILL:
        case PollState::KEEPALIVE:
            continue;
        case PollState::CONNECTING:
        case PollState::WRITE:
        case PollState::READ_WRITE:
            FD_SET(fd, &w);
            break;
        case PollState::READ:
            if (c->wantToSend())
                FD_SET(fd, &w);
            break;
#ifdef USE_GNUTLS
        case PollState::TLS_HANDSHAKE:
#endif
        case PollState::NONE:
            break;
        case PollState::READ_BLOCKED:
            c->setState(c->checkReadBlock());
            if (c->wantToSend())
                FD_SET(fd, &w);
            FD_SET(fd, &e);
            if (fd > max)
                max = fd;
            continue;
        }

        // Always check for readability (if closed by peer) and error:
        FD_SET(fd, &r);
        FD_SET(fd, &e);

        if (fd > max)
            max = fd;
    }

    // Check keep-alive connection since they may be closed by peer.
    for (auto p : keepaliveCache) {
        FD_SET(p.second, &r);
        FD_SET(p.second, &e);
        if (p.second > max)
            max = p.second;
    }

    return max;
}

#ifdef USE_THREADS
thread_local
#endif
volatile bool Engine::yield_called = false;
bool Engine::max_open_fd_reached = false;

void Engine::handleMaxOpenFdReached() {
    // TODO thread safe
    if (!keepaliveCache.empty()) {
        auto it = keepaliveCache.begin();
        Socket::closeSocket(it->second);
#ifdef USE_GNUTLS
        auto p = tls_session_cache.find(it->second);
        if (p != tls_session_cache.end())
            tls_session_cache.erase(p);
#endif
        keepaliveCache.erase(it);
        max_open_fd_reached = false;
        return;
    }
    if (connectionStore.size() > 100) {
        // Try to find a client socket to remove
        for (auto &p : connectionStore)
            if (auto s = dynamic_cast<SocketConnection *>(p.second)) {
                warn_log() << "Out of file descriptors, will remove "
                           << s->id();
                s->closeMe();
                max_open_fd_reached = false;
                return;
            }
    }
    // Close some random file descriptor
    warn_log() << "Out of file descriptors, possible fd leak";
    static int fd_to_remove = 10;
    while (++fd_to_remove < 65536) {
        if (connectionStore.find(fd_to_remove) == connectionStore.end()) {
            if (!Socket::closeSocket(fd_to_remove)) {
                warn_log() << "Possible fd leak, closed fd " << fd_to_remove;
                max_open_fd_reached = false;
                return;
            }
        }
    }
    warn_log() << "Out of file descriptors, cannot recover";
    fd_to_remove = 10;
}

// Handle network events for max_time seconds or until yield is called.
bool Engine::run(double max_time) {
    deadline = timeAfter(max_time);
    yield_called = false;

    for (auto p : connectionStore)
        if (p.second->hasExpired(deadline))
            killConnection(p.first);

    while (!yield_called) {

        if (reclaimConnections())
            continue; // Recheck after callbacks

        fd_set readFds, writeFds, errFds;
        int max_fd = setFds(readFds, writeFds, errFds);

        struct timeval timeout;
        {
            double time_left = secondsTo(deadline);
            long us = static_cast<long>(1e6*time_left);
            if (us <= 0)
                return true;
            timeout.tv_sec = us/1000000;
            timeout.tv_usec = us%1000000;
        }

        int res = select(max_fd + 1, &readFds, &writeFds, &errFds, &timeout);
        if (res < 0) {
            if (fatalSelectError())
                return false;
        } else {
            doFds(readFds, writeFds, errFds, max_fd);
            if (max_open_fd_reached)
                handleMaxOpenFdReached();
        }
    }
    reclaimConnections();
    return true;
}

void Engine::doFds(const fd_set &r, const fd_set &w, const fd_set &e, int max) {

    for (auto it=keepaliveCache.begin(); it != keepaliveCache.end(); ) {
        int fd = it->second;
        if (FD_ISSET(fd, &r) || FD_ISSET(fd, &e)) {
            log() << "close keepalive socket " << fd;
#ifdef USE_GNUTLS
            auto p = tls_session_cache.find(fd);
            if (p != tls_session_cache.end()) {
                gnutls_deinit(p->second);
                tls_session_cache.erase(p);
            }
#endif
            Socket::closeSocket(fd);
            it = keepaliveCache.erase(it);
        } else
            ++it;
    }

    for (int fd=0; fd<=max; ++fd) {
        // We cannot loop over connectionStore since it may
        // be modified in all sorts of ways within the loop.
        auto p = connectionStore.find(fd);
        if (p == connectionStore.end())
            continue;

        if (FD_ISSET(fd, &e)) {
            err_log() << "socket error " << fd;
            killConnection(fd);
            continue;
        }
        Socket *conn = p->second;

        bool readable = FD_ISSET(fd, &r);
        bool writable = FD_ISSET(fd, &w);

        if (!readable && !writable)
            continue;

        SocketConnection *c = dynamic_cast<SocketConnection *>(conn);

        if (!c) {
            // Is it a server connection?
            if (ServerSocket *srv =
                dynamic_cast<ServerSocket *>(conn)) {
                // New connection on listen socket
                handleIncoming(srv);
                continue;
            }
            err_log() << "event on non-client connection";
            killConnection(fd);
            continue;
        }

        if (readable && (c->state() == PollState::READ ||
                         c->state() == PollState::READ_WRITE)) {
            // React to reading before considering writing:
            c->setState(c->doRead(fd));
            // If it switches to sending, we'll probably be able to send some data
            // immediately, saving us a select roundtrip.
            if (c->state() == PollState::WRITE) {
                writable = true;
            } else {
                continue;
            }
        }

        if (writable) {
            PollState s;
            if (c->state() == PollState::CONNECTING) {
                if (c->inError()) {
                    err_log() << "async connect failed";
                    killConnection(fd);
                    continue;
                }
#ifdef USE_GNUTLS
                if (c->use_tls) {
                    if (!c->init_tls_client(x509_cred[0], !ca_bundle.empty())) {
                        log() << "cannot enable TLS" << fd;
                        killConnection(fd);
                        continue;
                    }
                    int ret = c->try_tls_handshake();
                    if (ret >= 0) {
                        s = c->connected();
                    } else if (gnutls_error_is_fatal(ret)) {
                        err_log() << "TLS handshake failure: " << gnutls_strerror(ret);
                        killConnection(fd);
                        continue;
                    } else {
                        s = PollState::TLS_HANDSHAKE;
                    }
                } else
#endif
                s = c->connected();
            } else {
                s = c->doWrite();
            }
            c->setState(s);
            continue;
        }

#ifdef USE_GNUTLS
        if (c->state() == PollState::TLS_HANDSHAKE) {
            int ret = c->try_tls_handshake();
            if (ret >= 0) {
                c->setState(c->connected());
            } else if (gnutls_error_is_fatal(ret)) {
                err_log() << "TLS handshake failure: " << gnutls_strerror(ret);
                killConnection(fd);
            }
            continue;
        }
#endif

        // The socket is readable, but we don't want to read it
        // since state is neither READ nor READ_WRITE.
        // The connection may have been closed by peer.
        PollState s = c->doRead(fd);
        c->setState(s);
    }

}

bool Engine::fatalSelectError() {
    if (Socket::isTempError()) {
        errno_log() << "select interrupt";
        return false;
    }

    // Oops, we've done something really wrong: an invalid
    // filedescriptor may have been added to the poll set.
    // Find it and remove it, then try again.
    errno_log() << "select error";

    for (auto p : connectionStore)
        if (p.second->inError()) {
            p.second->setState(PollState::CLOSE);
            killConnection(p.first);
            return false;
        }

    for (auto p=keepaliveCache.begin(); p != keepaliveCache.end(); ++p)
        if (Socket::socketInError(p->second)) {
#ifdef USE_GNUTLS
            auto it = tls_session_cache.find(p->second);
            if (it != tls_session_cache.end()) {
                gnutls_deinit(it->second);
                tls_session_cache.erase(it);
            }
#endif
            Socket::closeSocket(p->second);
            keepaliveCache.erase(p);
            return false;
        }

    // Don't know what's wrong, cannot recover.
    return true;
}

void Engine::killConnection(int fd) {
    auto p = connectionStore.find(fd);
    if (p != connectionStore.end()) {
        Socket *conn = p->second;
        SocketConnection *c = dynamic_cast<SocketConnection *>(conn);
        if (conn->state() == PollState::KEEPALIVE) {
            std::string label = conn->cacheLabel();
            if (!label.empty()) {
                log() << "keep-alive socket " << fd << " to " << label;
                keepaliveCache.insert(std::make_pair(label, fd));
#ifdef USE_GNUTLS
                if (c && c->is_tls())
                    tls_session_cache[fd] = c->cache_session();
#endif
                // Otherwise the socket will get closed when conn is deleted:
                conn->setSocket(-1);
            }
        }
        if (Task *task = conn->owner()) {
            if (c)
                task->connRemoved(c);
            else if (ServerSocket *srv = dynamic_cast<ServerSocket *>(conn))
                task->serverRemoved(srv);
        }
        delete conn;
        connectionStore.erase(p);
    }
}

std::set<Socket *> Engine::findSockByTask(const Task *t) const {
    std::set<Socket *> cset;
    for (auto p : connectionStore)
        if (p.second->owner() == t)
            cset.insert(p.second);
    return cset;
}

void Engine::wakeUpByTask(Task *t) {
    for (auto p : connectionStore)
        if (SocketConnection *c = dynamic_cast<SocketConnection *>(p.second))
            if (c->owner() == t && c->state() == PollState::NONE)
                c->setState(c->connected());
}

bool Engine::wakeUpConnection(SocketConnection *s) {
    if (s->state() != PollState::NONE)
        return false;
    s->setState(s->connected());
    return true;
}

void Engine::deleteConnByTask(const Task *task) {
    std::set<int> toRemove;
    for (auto p : connectionStore)
        if (p.second->owner() == task)
            toRemove.insert(p.first);
    for (auto fd : toRemove)
        killConnection(fd);
}

void Engine::cancelConnection(SocketConnection *s) {
    killConnection(s->socket());
}

void Engine::handleIncoming(ServerSocket *server) {
    SocketConnection *client = server->incoming();
    if (!client) {
        return;
    }
    int newfd = client->socket();

#ifdef USE_GNUTLS
    log() << "Socket " << newfd << " TLS=" << server->tlsKey();
    if (server->tlsKey()) {
        if (!client->init_tls_server(x509_cred[server->tlsKey()], priority_cache)) {
            log() << "cannot enable TLS" << newfd;
            delete client;
            return;
        }
        client->enableTLS();
        int ret = client->try_tls_handshake();
        if (ret >= 0) {
            client->setState(client->connected());
        } else if (gnutls_error_is_fatal(ret)) {
            err_log() << "TLS handshake has failed: " << gnutls_strerror(ret);
            delete client;
            return;
        } else {
            client->setState(PollState::TLS_HANDSHAKE);
        }
    } else
#endif
    client->setState(client->connected());
    connectionStore[newfd] = client;
    server->owner()->connAdded(client);
}

void Engine::addConnected(SocketConnection *conn) {
    dbg_log() << "addConnected " << conn->socket();
    connectionStore[conn->socket()] = conn;
    conn->owner()->connAdded(conn);
    conn->setState(conn->connected());
}
