#include <stdlib.h>
#include <unistd.h>
#include <fstream>

#include <framework/task.h>
#include <framework/socketreceiver.h>

/* Example 008

Demonstration of a task that creates workers (subproceses).
The Boss task creates three subprocesses, each running a Worker task.
Each Worker will do some processing and then exit.
The Boss wil exit when all worker processes are done.

This example does not work on Windows.

*/


class Worker : public Task {
public:

    Worker(const std::string label) : Task(label) {
    }

    ~Worker() override {
        log() << "Goodbye from " << label();
    }

    double start() override;

private:
};

double Worker::start() {
    log() << "Worker PID " << getpid() << " started.";
    srand(static_cast<unsigned int>(getpid()));
    unsigned long count = 0;
    while (true) {
        int n = rand();
        ++count;
        if (n % 10000000 == 3333333) {
            setResult(std::to_string(count) + " tries, result " +
                      std::to_string(n));
            break;
        }
    }
    return 0.0;
}

class Boss : public Task {
public:

    Boss(unsigned int no_workers) :
        Task("Boss"),
        tot_no_workers(no_workers) {
    }

    ~Boss() override {
    }

    // After creating a child process, the eventloop will call our
    // createWorkerTask method to get a task to run in the new process.
    Task *createWorkerTask(unsigned int wno) override {
        std::string name = "w" + std::to_string(wno);
        log() << "Will create worker " << name;
        return new Worker(name);
    }

    double start() override;

    void processFinished(int pid, int wstatus) override {
        log() << "OK, end of " << pid << " status " << wstatus;
        if (++wdone == tot_no_workers)
            setResult("all done");
    }
private:
    const unsigned int tot_no_workers;
    unsigned int wdone = 0;
};

double Boss::start() {
    // Messages and sockets can be passed between parent and worker processes
    // through "channels". We don't use channels in this example.

    // The createWorker call instructs the eventloop to create a child process
    // and run a new eventloop in that process. Our createWorkerTask method
    // will be called in the chid process to provide a task for the eventloop
    // running in the child process.
    // First parameter to createWorker is a file name for the worker's log.
    // The second parameter is the number of channels between parent and worker.
    for (unsigned int n=0; n < tot_no_workers; ++n)
        createWorker("w" + std::to_string(n) + ".log", 0);

    return 0;
}

int main(int , char *[]) {
    EventLoop eventloop("EventLoop");
    eventloop.addTask(new Boss(3));
    eventloop.runUntilComplete();
    return 0;
}
