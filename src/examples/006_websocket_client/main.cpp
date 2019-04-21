#include <framework/eventloop.h>
#include "winnerclient.h"

/* Example 006

A websocket client talking to the server of example 005.

*/

int main(int , char *[]) {
    EventLoop::runTask(new WinnerClient("127.0.0.1", 8080, "/winner"));
    return 0;
}
