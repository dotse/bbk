#include <http/webservertask.h>
#include <framework/eventloop.h>

/* Example 003

Start a webserver on port 8080. Write log to stderr.
From a web browser, you can retrieve the URLs

  http://127.0.0.1:8080/getTime
  http://127.0.0.1:8080/getStats
  http://127.0.0.1:8080/shutdown

*/

class WebServer : public WebServerTask {
public:
    WebServer(const std::string &cfg) :
        WebServerTask("WebServer", cfg) {
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
        // Send current time
        conn->sendHttpResponse(headers("200 OK"), "text/plain", dateString());
    else if (uri == "/getStats")
        // Send number of requests since server was started.
        conn->sendHttpResponse(headers("200 OK"), "text/plain",
                               std::to_string(tot_no_requests));
    else if (uri == "/shutdown") {
        conn->sendHttpResponse(headers("200 OK"), "text/plain",
                               "server shutdown");
        // This will terminate the task, i.e. shut the server down. Normally, of
        // course, clients wouldn't be able to do this on a prduction server.
        setResult("");
    } else
        conn->sendHttpResponse(headers("404 Not Found"), "text/plain",
                               "unknown service");
    return HttpState::WAITING_FOR_REQUEST;
}

int main(int , char *[]) {
    // Listen on port 8080. To listen on the standard port 80, we'd have to
    // run as a privileged user.
    EventLoop::runTask(new WebServer("listen 8080"));
    return 0;
}
