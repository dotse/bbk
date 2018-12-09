#include <http/httprequestengine.h>
#include <json11/json11.hpp>
#include <framework/eventloop.h>

class MainTask : public Task {
public:
    MainTask() : Task("Main") {
        bot = new HttpRequestEngine("MyBot", HttpHost("127.0.0.1", 8080),
                                    0, 3);
        //bot = new HttpRequestEngine("MyBot", "lab04.bredbandskollen.se");
    }

    ~MainTask() {
    }

    double start() override {
        dbg_log() << "starting";

        addNewTask(bot, this);

        bot->startObserving(this);
        bot->getJob(this, "Ticks " + std::to_string(++request_no),
                    "/chunk?ticks=10");
        bot->getJob(this, "Count " + std::to_string(++request_no),
                    "/getStats");
        killChildTaskWhenFinished();
        return 3.0;
    }

    double timerEvent() override {
        dbg_log() << "timerEvent";
        if (request_no < 10)
            bot->getJob(this, "Ticks " + std::to_string(++request_no),
                        "/chunk?ticks=10");
        bot->getJob(this, "Count " + std::to_string(++request_no),
                    "/getStats");
        if (request_no < 20)
            return 3.0;
        else
            return 0.0;
    }

    void taskFinished(Task *task) override {
        log() << "Oops, task " << task->label() << " died.";
        if (task == bot) {
            bot = nullptr;
            setResult("Failed");
        }
    }

    void handleExecution(Task *task, const std::string &name) override {
        log() << task->label() << " event: " << name;
        if (task != bot)
            return;
        if (bot->httpStatus()) {
            log() << "Exit job " << name << " result: " << bot->contents();
            ++ok_jobs;
            if (ok_jobs == 20)
                setResult("All done.");
        } else {
            log() << "Exit job " << name << " failed, will retry";
            bot->redoJob();
        }
    }
private:
    unsigned int request_no = 0, ok_jobs = 0;
    HttpRequestEngine *bot = nullptr;
    std::string _ticket;
};

int main(int , char *[]) {
    EventLoop eventloop("EventLoop");
    eventloop.addTask(new MainTask());
    eventloop.runUntilComplete();
    return 0;
}
