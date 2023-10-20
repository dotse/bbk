// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

/*! \file */

/// \enum PollState
/// After doing an operation on a socket, a PollState must be returned to the
/// network engine to describe what you want it to do next with the socket.
enum class PollState {
    NONE, /**< Do nothing right now, but keep socket open for later. */
    READ_BLOCKED, /**< Don't check for incoming data/close, but check for
                       writability if wantToSend(). */
    CONNECTING, /**< Wait for asynchronous connect to complete. */
#ifdef USE_GNUTLS
    TLS_HANDSHAKE, /**< Wait for TLS handshake to complete. */
#endif
    CLOSE, /**< Close the socket gracefully. */
    KEEPALIVE, /**< Put the connected socket in keep-alive cache. */
    KILL, /**< Terminate connection immediately, discarding buffers. */
    READ, /**< Check for incoming data/close. */
    WRITE, /**< Check for close, and for writability. */
    READ_WRITE /**< Check for incoming data/close, and for writability. */
};
