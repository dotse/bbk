#include <framework/eventloop.h>
#include "echoclienttask.h"

/* Example 011

Write a few messages to an echo server, check the result.
Make sure the server from example 010 is running while you run this client.

*/

int main(int , char *[]) {
    EventLoop::runTask(new EchoClientTask("127.0.0.1", 1400));
    return 0;
}
