#include <fstream>

#include <framework/eventloop.h>
#include <framework/synchronousbridge.h>

#include "webserver.h"

/* Example 020: Demonstration of the agent/bridge/client concept.

This program contains a CLI (command line interface) to a WebServer task.
It must be run in a terminal. The CLI is rather pointless, it just writes
the latest URL fetched from the WebServer in a single line in the terminal.
However, the same technique can be used to implement real user interfaces,
e.g. a GUI or a web interface.

A log is written to log.txt.

The WebServer task is an _agent_, i.e. it can communicate with a client that
does not run in the eventloop. The client is the CLI which is implemented by
the MyClient class below.

The communication between the agent and the client is facilitated by a _bridge_
which is a task running in the eventloop. The bridge can pass messages from
the agent to the client and from the client to the agent. Each message is a
single string.

The bridge class is a subclass of the BridgeTask class, specifying the API from
the agent's point of view. Since the client is not a Task (it could be anything
really), different BridgeTask sublasses have different means of communicating
with the client. However, the agent code does not depend on what client is used.

In this example, the agent and the client run in the same process. The most
common case though is that the client runs in another process or at least not
in the same thread as the eventloop.

To test the CLI client, run it in a terminal. Then retrieve a few URLs from the
agent, which runs on port 8080, e.g.

  http://127.0.0.1:8080/getTime
  http://127.0.0.1:8080/getStats
  http://127.0.0.1:8080/hi

The program will (as a demonstration) terminate after a few requests.

*/

class MyClient : public SynchronousClient {
public:
    // initial messages to the agent shall be pushed onto return_msgs.
    void initialMsgToAgent(std::deque<std::string> &return_msgs) override;

    // msg is a new message from the agent.
    // push any return messages onto return_msgs.
    void newEventFromAgent(std::deque<std::string> &return_msgs,
                           const std::string &msg) override;
private:
    std::string last_msg;
    unsigned int msgCount = 0;
};

void MyClient::initialMsgToAgent(std::deque<std::string> &return_msgs) {
    return_msgs.push_back("client ready");
}

void MyClient::newEventFromAgent(std::deque<std::string> &return_msgs,
                                  const std::string &msg) {
    ++msgCount;

    // Move back to leftmost position in the terminal
    std::cerr << std::string(last_msg.size(), '\010') << msg;
    if (last_msg.size() > msg.size()) {
        // New message is shorter, erase last part of old message in terminal
        size_t n = static_cast<size_t>(last_msg.size()-msg.size());
        std::cerr << std::string(n, ' ') << std::string(n, '\010');
    }
    last_msg = msg;

    // Just as a demonstration, we will send a termination request to the agent
    // after having received a few messages.
    if (msgCount == 4)
        return_msgs.push_back("quit");

    if (isTerminateMessage(msg)) {
        // Agent gone.
        std::cerr << "\nBye." << std::endl;
    }
}

int main(int , char *[]) {
    std::ofstream log("log.txt");
    Logger::setLogFile(log);
    EventLoop eventloop("EventLoop");
    WebServer *agent = new WebServer("listen 8080");
    MyClient *client = new MyClient();
    eventloop.addTask(new SynchronousBridge(agent, client));
    eventloop.runUntilComplete();
    return 0;
}
