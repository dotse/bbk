// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <algorithm>
#include "socket.h"

#ifdef USE_GNUTLS
#include <gnutls/gnutls.h>
#endif

/// \brief
/// This class implements low-level socket connection operations.
/// Inherit from it to implement protocols like HTTP.
///
/// A SocketConnection object represents a socket connection. Inherit from this
/// class to implement a "protocol" for the connection, i.e. when/what to
/// read/write through the connection.
///
/// Each SocketConnection is owned by a Task object. SocketConnection objects
/// will be added to the network Engine (a part of the EventLoop) through the
/// task's addConnection method.
///
/// The subclass will be notified through callback functions when
/// anything happens on the socket, i.e. if the socket has been closed, if data
/// has arrived, or if the socket is writable.
/// You define the callback functions by overloading the below virtual functions.
/// All operations will be performed asynchronously (non-blocking) except for
/// dns lookups, which are performed synchronously (blocking).
///
/// Note! You _must_ create the SocketConnection objects with new. The ownership
/// will then be passed to the network engine.
/// You are not allowed to delete a SocketConnection object.
/// We will delete it when the connection has been closed, which will be
///
///  1) If you order us to close it by returning CLOSE, KEEPALIVE or KILL from
///     a callback, or
///
///  2) After the closedByPeer callback, if the connection has been
///     closed by peer.
///
///  3) After the connectionFailed callback, if the connection couldn't
///     be established in the first place.
///
/// The owner task will be notified before the object is deleted.
class SocketConnection : public Socket {
    friend class Engine;
public:
    /// Create a SocketConnection owned by the given Task. The network Engine
    /// will connect to the given hostname/port, and notify through the below
    /// method when the connection is ready.
    /// If iptype is 4, prefer ipv4. If iptype is 6, prefer ipv6.
    SocketConnection(const std::string &label, Task *owner,
                     const std::string &hostname, uint16_t port,
                     uint16_t iptype = 0, struct addrinfo *local_addr=nullptr);

#ifdef USE_GNUTLS
    virtual ~SocketConnection() override {
        if (tlsInitialized()) {
            gnutls_deinit(session);
        }
    }

    /// Notify that the connection will be encrypted (SSL).
    void enableTLS() {
        use_tls = true;
    }

    /// Return true if the connection will be encrypted (SSL).
    bool is_tls() const {
        return use_tls;
    }

    /// Store SSL session in cache.
    gnutls_session_t cache_session() {
        session_initialized = false;
        return session;
    }

    /// Reuse cached SSL session.
    void insert_cached_session(gnutls_session_t &old_session) {
        session_initialized = true;
        use_tls = true;
        session = old_session;
        gnutls_transport_set_ptr(session,
                                 static_cast<gnutls_transport_ptr_t>(this));
    }
#endif

    /// \brief
    /// Will be called when the connection is established.
    ///
    /// Override it to start sending data when the connection is ready.
    ///
    /// If a connection couldn't be established,
    /// SocketConnection::connectionFailed will be called instead.
    /// You must not return PollState::CONNECTING.
    virtual PollState connected() {
        return PollState::CLOSE;
    }

    /// Will be called if the connection couldn't be established.
    virtual void connectionFailed(const std::string &err_msg) {
        log() << "connection failed: " << err_msg;
    }

    /// Called when the socket has been closed by peer:
    /// May be called in states READ, WRITE, READ_WRITE
    virtual void closedByPeer();

    /// \brief
    /// Callback, called when data has arrived; len > 0.
    ///
    /// May be called in states READ, READ_WRITE. Override this method to
    /// handle the data. Return value should be the new state.
    ///
    /// The buffer is owned by the implementation. However, you are allowed
    /// to modify the contents of the buffer and you may also use
    /// it in the asyncSendData call.
    virtual PollState readData(char *, size_t ) {
        return PollState::CLOSE;
    }

    /// \brief
    /// Peer has sent data when it wasn't supposed to.
    ///
    /// If peer sends data when we're not in state READ/READ_WRITE, this
    /// function will be called. Default is for the socket to be closed.
    /// If you return READ, any remaining async_send data will be discarded.
    virtual PollState unexpectedData(char *buf, size_t len);

    /// \brief
    /// Callback, called when socket is writable.
    ///
    /// May be called in states PollState::WRITE and PollState::READ_WRITE.
    /// Override it to write data.
    /// Return value should be the new state. Do not return PollState::WRITE or
    /// PollState::READ_WRITE except after a write operation where
    /// all data couldn't be sent immediately.
    virtual PollState writeData() {
        return PollState::CLOSE;
    }

    /// \brief
    /// Try to send len bytes from the given buffer. Return the amount sent.
    ///
    /// More accurately, this method will return the number of bytes that
    /// could be copied into the socket's send buffer.
    ///
    /// Please understand that the return value might be < len.
    /// To safely get all data sent, you should use the
    /// SocketConnection::asyncSendData
    /// function instead. However, if you need to send large
    /// amounts of data ("large" as in "no upper limit") as fast
    /// as possible, this is the function to use.
    size_t sendData(const char *buf, size_t len);

#ifdef USE_WEBROOT
    size_t sendFileData(int fd, size_t len);
#endif

    /// \brief
    /// Send data to peer as soon as possible.
    ///
    /// Helper function which you may call only during the
    /// execution of the callbacks SocketConnection::connected,
    /// SocketConnection::readData, and SocketConnection::writeData.
    ///
    /// Send len bytes from the given buffer.
    /// The callback SocketConnection::closedByPeer
    /// might be executed before all data was sent.
    ///
    /// If you need to send "unlimited" amounts of data, you cant use
    /// this method; instead you must use SocketConnection::sendData.
    void asyncSendData(const char *buf, size_t len);

    /// \brief
    /// Send data to peer as soon as possible.
    ///
    /// Helper function which you may call only during the
    /// execution of the callbacks SocketConnection::connected,
    /// SocketConnection::readData, and SocketConnection::writeData.
    ///
    /// Send len bytes from the given buffer.
    /// The callback SocketConnection::closedByPeer
    /// might be executed before all data was sent.
    ///
    /// If you need to send "unlimited" amounts of data, you cant use
    /// this method; instead you must use SocketConnection::sendData.
    void asyncSendData(const std::string data) {
        asyncSendData(data.c_str(), data.size());
    }

    /// \brief
    /// Return number of bytes left to send after calling
    /// SocketConnection::asyncSendData.
    size_t asyncBufferSize() const {
        return to_send.size();
    }

    /// Number of bytes sent by current thread
    static uint64_t totBytesSent() {
        return tot_bytes_sent;
    }

    /// Number of bytes recieved by current thread
    static uint64_t totBytesReceived() {
        return tot_bytes_received;
    }

    /// \brief
    /// Reset counter for SocketConnection::totBytesSent
    /// and SocketConnection::totBytesReceived.
    static void resetByteCounter() {
        tot_bytes_sent = 0;
        tot_bytes_received = 0;
    }

    /// Return peer's IP address.
    const std::string &peerIp() const {
        return peer_ip;
    }

    /// Return peer's port number.
    uint16_t peerPort() const {
        return peer_port;
    }

    /// Enable debug output of data sent and received.
    void dbgOn(bool b = true) {
        debugging = b;
    }

    /// Return true if socket debugging is enabled.
    bool dbgIsOn() {
        return debugging;
    }
protected:

    /// If fd is the socket descriptor of an already established connection,
    /// you may let us manage the connection by calling this constructor.
    /// fd will probably be a client socket connected through a ServerSocket.
    SocketConnection(const std::string &label, Task *owner, int fd,
                     const char *ip, uint16_t port);

    /// Send a "message" to owner task. It will be executed as
    /// Task::msgFromConnection(this, msg) in the owner task.
    /// This is useful if you want to create SocketConnection
    /// subclasseses that work with any Task.
    PollState tellOwner(const std::string &msg);

private:
    // Create a connection to the given host. Will be executed asynchronously,
    // then one of the callbacks connected / connectionFailed will be called.
    SocketConnection(Task *owner, const std::string &hostname,
                     unsigned int port);

    SocketConnection(const SocketConnection &);

    // Will return false if connection fails immediately, e.g. if DNS
    // lookup fails. Otherwise callback "connected" or "connectionFailed"
    // will be called - perhaps even before this call returns:
    bool asyncConnect();

    PollState doRead(int fd);

    // In states READ and READ_BLOCKED, this will be called to check if
    // we the connection should be checked for writability too.
    // If you override this, make sure to return true if asyncBufferSize() > 0.
    bool wantToSend() override {
        return !to_send.empty();
    }

    PollState doWrite() {
        if (to_send.empty())
            return this->writeData();
        if (size_t written = sendData(to_send.c_str(), to_send.size()))
            to_send.erase(0, written);
        return state();
    }

    std::string to_send;
    char socket_buffer[100000];

    std::string peer_ip;
    struct addrinfo *local_ip;
    uint16_t peer_port;
    uint16_t prefer_ip_type;

#ifdef USE_THREADS
    thread_local
#endif
    // Per thread byte counters.
    static uint64_t tot_bytes_sent, tot_bytes_received;
    bool debugging = false;
#ifdef USE_GNUTLS
    bool use_tls = false;
    bool session_initialized = false;
    bool tls_send_pending = false;
    void setSessionInitialized() {
        session_initialized = true;
    }
    bool tlsInitialized() {
        return session_initialized;
    }
    bool init_tls_server(gnutls_certificate_credentials_t &x509_cred,
                         gnutls_priority_t &priority_cache);
    bool init_tls_client(gnutls_certificate_credentials_t &x509_cred,
                         bool verify_cert);
    int try_tls_handshake() {
        dbg_log() << "TLS handshake socket " << socket();
        return gnutls_handshake(session);
    }
    static ssize_t tls_pull_static(gnutls_transport_ptr_t self,
                                   void *buf, size_t len) {
        return static_cast<SocketConnection *>(self)->tls_pull(buf, len);
    }
    static ssize_t tls_push_static(gnutls_transport_ptr_t self,
                                   const void *buf, size_t len) {
        return static_cast<SocketConnection *>(self)->tls_push(buf, len);
    }
    ssize_t tls_pull(void *buf, size_t len);
    ssize_t tls_push(const void *buf, size_t len);
    gnutls_session_t session;
#endif
};
