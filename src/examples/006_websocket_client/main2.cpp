#include <framework/eventloop.h>
#include "winnerclient.h"

/* Example 006

A websocket client talking to the server of example 005.
This is an alternative version of the main function, where you can start the
client with the command line parameters

    --logfile=
    --host=
    --port=
    --url=

To use this version, update the Makefile to use main2.cpp instead of main.cpp.

*/

int main(int argc, char *argv[]) {
    TaskConfig cfg;

    cfg.set("host", "127.0.0.1");
    cfg.set("port", "8080");
    cfg.set("url", "/winner");
    cfg.parseArgs(argc, argv);

    // Use value of parameter --logfile as filename to save the log if it exists
    // and isn't equal to "-".
    std::ofstream log;
    cfg.openlog(log);

    auto task = new WinnerClient(cfg.value("host"),
                                 std::stoi(cfg.value("port")),
                                 cfg.value("url"));
    EventLoop::runTask(task);
    return 0;
}
