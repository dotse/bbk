#include <framework/eventloop.h>
#include "winner.h"

/* Example 005

Start a webserver on port 8080. Write log to stderr.
The client can open a websocket connection on

    ws://127.0.0.1:8080/winner

The client can then send text messages of the format

    name score

and the server will remember the names and scores.
If the client sends the text message "winner", the
server will respond with the name with the highest score.

E.g. the client sends the five messages

    Bill 12
    Steve 19
    Linus 33
    Ken 27
    winner

and the server responds

    Linus

*/

int main(int , char *[]) {
    EventLoop::runTask(new Winner("listen 8080"));
    return 0;
}
