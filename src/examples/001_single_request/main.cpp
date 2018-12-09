#include <http/singlerequest.h>
#include <framework/eventloop.h>

/* Example 001

Fetch http://frontend.bredbandskollen.se/api/servers, then exit.
Write debug log to stderr.

*/

int main(int , char *[]) {
    Task *t = new SingleRequest("MyRequest",
                                "frontend.bredbandskollen.se",
                                "/api/servers")
    EventLoop::runTask(t);
    // Note: the object t will be deleted by the event loop.
    return 0;
}
