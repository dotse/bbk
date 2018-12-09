#include <framework/eventloop.h>
#include "echoservertask.h"

/* Example 010

Implement a server for the echo network protocol, i.e.
just echo all received data back to the client.
Write log to stderr.

The class EchoServerTask takes a port number as a parameter.
Will accept clients on that port number.

To test it, start the server and open one or more other terminals.
In each of the terminals, run the command
   telnet 127.0.0.1 1400
and type a few lines of text. The server should echo back the text.

*/

int main(int , char *[]) {
    EventLoop::runTask(new EchoServerTask(1400));
    return 0;
}
