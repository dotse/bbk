#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <algorithm>

#include <http/webservertask.h>
#include <framework/eventloop.h>

/* Example 009

Create two threads, each running a WebServer in its own eventloop.
One will listen on port 8000, the other on 8080. To test, open URLs

http://127.0.0.1:8000/getTime
http://127.0.0.1:8080/getTime
http://127.0.0.1:8000/getStats
http://127.0.0.1:8080/getStats
http://127.0.0.1:8000/stop
http://127.0.0.1:8080/stop

in a web browser. The /stop URL will tell each server to exit. The test program
will exit when both servers have been stopped.

*/

class WebServer : public WebServerTask {
public:
    WebServer(const std::string &cfg, const std::string &name) :
    WebServerTask(name, cfg) {
    }

    HttpState newGetRequest(HttpServerConnection *,
                            const std::string &uri) override;
 private:
    unsigned long tot_no_requests = 0;
};

HttpState WebServer::newGetRequest(HttpServerConnection *conn,
                                   const std::string &uri) {
    ++tot_no_requests;
    log() << "URI: " << uri << " #" << tot_no_requests;
    if (uri == "/getTime")
        conn->sendHttpResponse(headers("200 OK"), "text/plain", dateString());
    else if (uri == "/getStats")
        conn->sendHttpResponse(headers("200 OK"), "text/plain",
                               std::to_string(tot_no_requests));
    else if (uri == "/stop")
        setResult("DONE");
    else
        conn->sendHttpResponse(headers("404 Not Found"), "text/plain",
                               "unknown service");
    return HttpState::WAITING_FOR_REQUEST;
}

int main(int , char *[]) {
    std::ofstream my_log("main.log"), log1("M8000.log"), log2("M8080.log");
    Logger::setLogFile(my_log);
    EventLoop loop;
    loop.spawnThread(new WebServer("listen 8000", "W8000"), "M8000", &log1);
    loop.spawnThread(new WebServer("listen 8080", "W8080"), "M8080", &log2);
    loop.runUntilComplete();
    return 0;
}
