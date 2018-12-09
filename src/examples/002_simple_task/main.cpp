#include <http/singlerequest.h>
#include <framework/eventloop.h>

/* Example 002

Fetch http://frontend.bredbandskollen.se/api/servers.
Write the result to stdout.
Write debug log to log.txt.

*/

class MainTask : public Task {
public:

    // Each task object has a name (or "label"), which is used (for example)
    // by the logger.
    MainTask(const std::string &name) : Task(name) {
        // Note: the constructor will be executed before the task has been added
        // to the eventloop.
        // DO NOT perform any actions that concern the eventloop here!
        // Instead, most of the initialisation should be performed from within
        // the start() method, which will be called by the eventloop when the
        // execution of this task starts.
    }

    double start() override {
        dbg_log() << "starting";

        // If child tasks still exist when this task is done,
        // we want then to be removed:
        killChildTaskWhenFinished();

        // The second parameter to addNewTask sets this task as
        // parent of the new task.
        addNewTask(new SingleRequest("MyRequest",
                                     "frontend.bredbandskollen.se",
                                     "/api/servers"), this);

        // First timer is to be called after 10.0 seconds.
        // If we return <= 0, no timer will be added.
        return 10.0;
    }

    double timerEvent() override {
        std::cerr << "Timeout after " << elapsed() << " seconds";

        // Tell the eventloop that this task has given up:
        setTimeout();

        // Return number of seconds until this method should be
        // called again, or <= 0 if you don't want it to be called again.
        return 0;
    }

    // Will be called when a child task is finished:
    void taskFinished(Task *task) override {
        log() << task->label() << " finished, ok=" << task->finishedOK();

        // Our only child task is a SingleRequest task, so the below cast will
        // succeed. We need the cast to be able to call httpStatus().
        if (SingleRequest *req = dynamic_cast<SingleRequest *>(task)) {
            log() << "Status: " << req->httpStatus();
            std::cout << "Result: " << req->result() << std::endl;
            // Calling setResult tells the eventloop that this task is done.
            setResult("OK");
        }
    }
};

int main(int , char *[]) {
    std::ofstream log("log.txt");
    // Note: the log file object must not be destroyed until either the
    // eventloop is finished, or until setLogFile is called again.
    Logger::setLogFile(log);
    EventLoop::runTask(new MainTask("Main Task"));
    return 0;
}
