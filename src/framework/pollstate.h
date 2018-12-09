// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

// What we want to do with a Socket object:
//   NONE - nothing right now, but keep it open for later
//   READ_BLOCKED - don't check for incoming data/close, but check for
//                  writability if wantToSend().
//   CONNECTING - waiting for asynchronous connect to complete
//   CLOSE - close it gracefully
//   KEEPALIVE - put the connected socket in keep-alive cache
//   KILL - terminate it immediately, discarding buffers
//   READ - check for incoming data/close
//   WRITE - check for close, and for writability
//   READ_WRITE - check for incoming data/close, and for writability

enum class PollState {
    NONE,
    READ_BLOCKED,
    CONNECTING,
#ifdef USE_GNUTLS
    TLS_HANDSHAKE,
#endif
    CLOSE,
    KEEPALIVE,
    KILL,
    READ,
    WRITE,
    READ_WRITE
};
