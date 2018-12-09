#include "echoserverconnection.h"

EchoServerConnection::EchoServerConnection(Task *task, int fd,
                                           const char *ip, uint16_t port) :
    SocketConnection("Echo Client Handler", task, fd, ip, port) {
}

PollState EchoServerConnection::connected() {
    log() << "New client " << id() << ", waiting for data";
    return PollState::READ;
}

PollState EchoServerConnection::readData(char *buf, size_t len) {
    // Just send the data back to the client
    asyncSendData(buf, len);

    if (len > 20) {
        dbg_log() << "Client " << id() << " said "
                  << std::string(buf, 20) << "... (" << len << " bytes)";
    } else {
        dbg_log() << "Client " << id() << " said " << std::string(buf, len);
    }

    if (asyncBufferSize() > 1000000) {
        // The client does not read data as fast as we receive it.
        // We have to stop reading until the outgoing buffer
        // has shrunk to a manageable size.
        dbg_log() << "Send buffer too large: " << asyncBufferSize();
        return PollState::READ_BLOCKED;
    }
    return PollState::READ;
}

PollState EchoServerConnection::checkReadBlock() {
    dbg_log() << "Check send buffer: " << asyncBufferSize();
    if (asyncBufferSize() > 100000)
        return PollState::READ_BLOCKED;
    return PollState::READ;
}
