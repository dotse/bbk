#include <framework/task.h>
#include <http/singlerequest.h>
#include <framework/eventloop.h>

/* Example 007

Simple demonstration of sending events (direct messages) between tasks.
The SenderTask will send a message to ReceiverTask each second.
The ReceiverTask will terminate after the third message.
The SenderTask will terminate after the ReceiverTask has terminated.

*/

class ReceiverTask;

class SenderTask : public Task {
public:
    SenderTask(ReceiverTask *task) : Task("SenderTask"), peer(task) {
    }
    double start() override;
    double timerEvent() override;
    void taskFinished(Task *task) override;

private:
    ReceiverTask *peer;
};

class ReceiverTask : public Task {
public:
    ReceiverTask() : Task("ReceiverTask") {
    }

    void handleExecution(Task *, const std::string &msg) override;
private:
    unsigned int msgCount = 0;
};

void ReceiverTask::handleExecution(Task *, const std::string &msg) {
    log() << "Event: " << msg;
    if (++msgCount == 3)
        setResult("Got Event");
}

double SenderTask::start() {
    // We have to register an "interest" in peer by calling the startObserving
    // method, otherwise the messages we try to send to it will be ignored.
    // Also, by doing this we will be notified (through taskFinished) if
    // the peer task dies; that is important since we must not keep the pointer
    // to the peer after it has been deleted.
    if (!startObserving(peer)) {
        log() << "Peer task does not exist.";
        setResult("Fail");
        return 0.0;
    }
    return 0.1;
}

double SenderTask::timerEvent() {
    executeHandler(peer, "Hi there!");
    return 1.0;
}

void SenderTask::taskFinished(Task *task) {
    if (task == peer) {
        peer = nullptr;
        log() << "Peer task dead, will exit.";
        setResult("Done.");
    }
}

int main(int , char *[]) {
    EventLoop eventloop("EventLoop");
    ReceiverTask *it = new ReceiverTask();
    eventloop.addTask(it);
    eventloop.addTask(new SenderTask(it));
    eventloop.runUntilComplete();
    return 0;
}
