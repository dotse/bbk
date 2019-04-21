// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#include <algorithm>
#include <stdexcept>
#include <csignal>

#include "eventloop.h"
#include "engine.h"
#include "task.h"
#include "socketconnection.h"
#include "serversocket.h"

#ifdef USE_THREADS
thread_local
#endif
volatile int EventLoop::got_signal = 0;

#ifdef _WIN32
void EventLoop::signalHandler(int signum) {
    EventLoop::interrupt();
    got_signal = signum;
}
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include "socketreceiver.h"
#include "workerprocess.h"

#ifdef USE_THREADS
thread_local
#endif
std::map<int, int> EventLoop::terminatedPIDs;
#ifdef USE_THREADS
thread_local
#endif
volatile int EventLoop::terminatedPIDtmp[100] = { 0 };
#ifdef USE_THREADS
thread_local
#endif
std::string EventLoop::openFileOnSIGHUP;

void EventLoop::signalHandler(int signum) {
    EventLoop::interrupt();
    if (signum != SIGCHLD) {
        got_signal = signum;
        return;
    }
    while (true) {
        int wstatus;
        pid_t pid = waitpid(-1, &wstatus, WNOHANG);
        if (pid <= 0)
            break;
        //Logger::log("signalHandler") << "Child PID " << pid
        //                             << " terminated, status " << wstatus;
        for (size_t i=0; i<sizeof(terminatedPIDtmp)/sizeof(int); i+=2)
            if (!terminatedPIDtmp[i]) {
                terminatedPIDtmp[i] = pid;
                terminatedPIDtmp[++i] = wstatus;
                return;
            }
        // Bad, shoudn't use terminatedPIDs asynchronously. Won't happen if
        // terminatedPIDtmp is large enough, i.e. twice number of chldren.
        EventLoop::terminatedPIDs[pid] = wstatus;
    }
}

int EventLoop::externalCommand(Task *owner, const char *const argv[]) {
    if (!argv[0])
        return false;
    pid_t chld = fork();
    if (chld < 0) {
        errno_log() << "cannot fork";
        return -1;
    }
    if (!chld) {
        // TODO: Close all file descriptors in child!
        int ret = execvp( argv[0], const_cast<char **>(argv) );
        exit(ret);
    }
    pidOwner[chld] = owner;
    return chld;
}

void EventLoop::daemonize() {
    if (fork() != 0)
        exit(0); // Close original process

    setsid(); // Detach from shell

    if (int fd = open("/dev/null", O_RDWR, 0) != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO)
            close(fd);
    }
}

WorkerProcess *EventLoop::createWorker(Task *parent,
                                       const std::string &log_file_name,
                                       unsigned int channels,
                                       unsigned int wno) {
    setLogFilename(log_file_name);
    auto logfile = new std::ofstream(log_file_name, std::ios::app);
    return createWorker(parent, logfile, channels, wno);
}

WorkerProcess *EventLoop::createWorker(Task *parent, std::ostream *log_file,
                                       unsigned int channels,
                                       unsigned int wno) {
    if (aborted())
        return nullptr;
    std::vector<int> array(2 * channels);
    int *pair_sd = array.data();
    for (unsigned int i = 0; i<channels; ++i)
        if (socketpair(AF_UNIX, SOCK_DGRAM, 0, pair_sd+2*i) < 0) {
            if (errno == EMFILE)
                Engine::notifyOutOfFds();
            errno_log() << "cannot create socketpair";
            return nullptr;
        }

    // TODO: second parameter: unsigned int noSocketReceivers, default 1
    //       different class for communication channels
    int ppid = getpid();

    pid_t wpid = fork();
    if (wpid < 0)
        return nullptr;

    if (wpid > 0) {
        // In parent (original) process
        log() << "started worker " << wpid;
        if (log_file)
            delete log_file;
        std::vector<SocketReceiver *> receivers;
        for (unsigned int i = 0; i<2*channels; i+=2) {
            close(pair_sd[i+1]);
            fcntl(pair_sd[i], F_SETFL, O_NONBLOCK);
            SocketReceiver *conn = new SocketReceiver(parent, pair_sd[i], wpid);
            if (!addServer(conn)) {
                err_log() << "Cannot create worker channel";
                while (i < 2*channels)
                    close(pair_sd[i++]);
                return nullptr;
            }
            log() << "Worker channel fd " << pair_sd[i];
            receivers.push_back(conn);
        }
        pidOwner[wpid] = parent;
        return new WorkerProcess(wpid, receivers);
    }

    // In worker (new child process)

    engine.childProcessCloseSockets();
    Task::supervisor = nullptr;

    // Set new log file for child process
    if (!log_file) {
        log_file = new std::ostringstream();
        log_file->clear(std::istream::eofbit);
    }
    setLogFile(*log_file);

    EventLoop eventloop("Worker");
    Task *worker = parent->createWorkerTask(wno);
    if (!worker) {
        err_log() << "Cannot create worker " << wno << ", will exit";
        exit(1);
    }
    eventloop.addTask(worker);
    for (unsigned int i = 0; i<2*channels; i+=2) {
        close(pair_sd[i]);
        fcntl(pair_sd[i+1], F_SETFL, O_NONBLOCK);
        SocketReceiver *chan = new SocketReceiver(worker, pair_sd[i+1], ppid);
        worker->newWorkerChannel(chan, i/2);
        worker->addServer(chan);
    }
    eventloop.runUntilComplete();
    parent->finishWorkerTask(wno);
    log() << "Exit" << std::endl;
    delete log_file;
    exit(0);
}

void EventLoop::killChildProcesses(int signum) {
    for (auto p : pidOwner)
        kill(p.first, signum);
}

#endif

EventLoop::~EventLoop() {
    removeAllTasks();
    Task::supervisor = nullptr;
    log() << "EventLoop finished.\n";
}

#ifdef USE_THREADS
void EventLoop::do_init(EventLoop *parent) {
    parent_loop = parent;
#else
void EventLoop::do_init() {
#endif
    if (Task::supervisor) {
        throw std::runtime_error("eventloop already exists");
    }
    Task::supervisor = this;
    got_signal = 0;
#ifdef USE_THREADS
    if (parent_loop)
        return;
#endif
#ifdef _WIN32
    // Windows specific sockets initialization
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
        throw std::runtime_error("cannot initialize network");
    signal(SIGTERM, signalHandler);
    signal(SIGABRT, signalHandler);
#else
    terminatedPIDs.clear();
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signalHandler;
    sigfillset(&sa.sa_mask);
    if (sigaction(SIGCHLD, &sa, nullptr) ||
        sigaction(SIGTERM, &sa, nullptr) ||
        sigaction(SIGINT, &sa, nullptr) ||
        sigaction(SIGHUP, &sa, nullptr) ||
        sigaction(SIGQUIT, &sa, nullptr) ||
        sigaction(SIGABRT, &sa, nullptr) ||
        sigaction(SIGPIPE, &sa, nullptr))
        errno_log() << "cannot add signal handler";
#endif
}

void EventLoop::addSignalHandler(int signum, void (*handler)(int, EventLoop &)) {
#ifdef _WIN32
    if (signal(signum, signalHandler) == SIG_ERR) {
        errno_log() << "cannot add handler for signal " << signum;
        return;
    }
#else
    if (signum == SIGCHLD) {
        err_log() << "must not set custom handler for SIGCHLD";
        return;
    }
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signalHandler;
    if (sigaction(signum, &sa, nullptr)) {
        errno_log() << "cannot add handler for signal " << signum ;
        return;
    }
#endif
    userSignalHandler.insert(std::make_pair(signum, handler));
}

void EventLoop::addTask(Task *task, Task *parent) {
    if (task && tasks.find(task) == tasks.end()) {
        tasks[task] = parent;
        if (parent)
            startObserving(parent, task);
        if (task->isFinished()) {
            // Task finished when executing its constructor
            notifyTaskFinished(task);
            return;
        }
        double ts = task->begin();
        log() << "Task " << task->label() << " timeout " << ts;
        if (ts > 0) {
            TimePoint t = timeAfter(ts);
            timer_queue.insert(std::make_pair(t, task));
            // Make sure we get back here when the timer expires:
            engine.resetDeadline(t);
        }
    } else {
        err_log() << "cannot add task";
    }
}

bool EventLoop::addConnection(SocketConnection *conn) {
    if (!conn)
        return false;

    if (tasks.find(conn->owner()) != tasks.end() &&
        engine.addClient(conn)) {
        conn->owner()->connAdded(conn);
        return true;
    }

    err_log() << "cannot add connection";
    return false;
}

bool EventLoop::addConnected(SocketConnection *conn) {
    if (!conn)
        return false;

    if (tasks.find(conn->owner()) != tasks.end()) {
        engine.addConnected(conn);
        return true;
    }

    err_log() << "cannot add connection";
    delete conn;
    return false;
}

bool EventLoop::addServer(ServerSocket *conn) {
    if (!conn)
        return false;

    if (tasks.find(conn->owner()) != tasks.end() &&
        engine.addServer(conn)) {
        conn->owner()->serverAdded(conn);
        return true;
    }

    err_log() << "cannot add server connection";
    return false;
}

// This is just a safeguard against buggy clients.
// Will be called _after_ object pointed to by task
// has been deleted, so don't dereference the pointer.
void EventLoop::taskDeleted(Task *task) {
    auto p = tasks.find(task);
    if (p != tasks.end()) {
        err_log() << "task owned by me deleted by client!";
        tasks.erase(p);
        engine.deleteConnByTask(task);
    }
}

bool EventLoop::startObserving(Task *from, Task *to) {
    if (tasks.find(from) == tasks.end() ||
        tasks.find(to) == tasks.end()) {
        err_log() << "startObserving: no such task";
        return false;
    }
    {
        auto p = observing.find(to);
        if (p == observing.end()) {
            std::set<Task *> tset { from };
            observing.insert(std::make_pair(to, tset));
        } else {
            p->second.insert(from);
        }
    }
    {
        auto p = observed_by.find(from);
        if (p == observed_by.end()) {
            std::set<Task *> tset { to };
            observed_by.insert(std::make_pair(from, tset));
        } else {
            p->second.insert(to);
        }
    }

    return true;
}

void EventLoop::getChildTasks(std::set<Task *> &tset, Task *parent) const {
    // parent is observing its children.
    auto p = observed_by.find(parent);
    if (p == observed_by.end())
        return;

    for (auto task : p->second)
        // task is observed by parent but isn't necessarily a child task.
        if (tasks.at(task) == parent)
            tset.insert(task);
}

void EventLoop::abortChildTasks(Task *parent) {
    std::set<Task *> tset;
    getChildTasks(tset, parent);
    for (auto task : tset) {
        task->was_killed = true;
        finishedTasks.insert(task);
    }
    engine.yield();
}

void EventLoop::abortTask(Task *task) {
    task->was_killed = true;
    finishedTasks.insert(task);
    engine.yield();
}

void EventLoop::wakeUpTask(Task *t) {
    engine.wakeUpByTask(t);
}

std::set<Socket *> EventLoop::findConnByTask(const Task *t) const {
    return engine.findSockByTask(t);
}

bool EventLoop::running(Task *task) {
    return tasks.find(task) != tasks.end();
}

void EventLoop::resetTimer(Task *task, double s) {
    _removeTimer(task);
    if (s < 0)
        return;
    if (s == 0)
        s = task->timerEvent();
    if (s > 0) {
        auto t = std::chrono::microseconds(toUs(s));
        timer_queue.insert(std::make_pair(timeNow()+t, task));
    }
}

void EventLoop::_removeTimer(Task *task) {
    for (auto it : timer_queue)
        if (it.second == task) {
            timer_queue.erase(it.first);
            break;
        }
}

void EventLoop::_removeTask(Task *task, bool killed) {
    {
        auto p = tasks.find(task);
        if (p == tasks.end())
            return; // Task already removed.
        tasks.erase(p);
    }

    task->setTerminated();
    task->was_killed = killed;
    log() << "remove task " << task->label();

    // Remove the task's connections
    engine.deleteConnByTask(task);

    // Remove the task's timer
    for (auto it : timer_queue)
        if (it.second == task) {
            timer_queue.erase(it.first);
            break;
        }

    // The dying task may be an observer:
    auto p1 = observed_by.find(task);
    if (p1 != observed_by.end()) {
        for (auto t : p1->second) {
            // task must no longer be observing t
            observing[t].erase(task);
            auto p = tasks.find(t);
            if (p->second == task) {
                // t is a child of the dying task. Mark it as an orphan:
                p->second = nullptr;
                if (task->kill_children)
                    abortTask(t);
            }
        }
        observed_by.erase(p1);
    }

    // Other tasks may be observing the dying task:
    auto p2 = observing.find(task);
    if (p2 != observing.end()) {
        for (auto t : p2->second) {
            // t must no longer be observing task
            observed_by[t].erase(task);
            // Notify t that an observed task has died
            t->taskFinished(task);
        }
        observing.erase(p2);
    }

#ifdef USE_THREADS
    if (task->is_child_thread)
        finished_threads.push(task);
    else
#endif
        delete task;
}

Task *EventLoop::nextTimerToExecute() {
    if (!timer_queue.empty() &&
        timer_queue.cbegin()->first <= timeNow()) {
        Task *task = timer_queue.cbegin()->second;
        timer_queue.erase(timer_queue.begin());
        return task;
    }
    return nullptr;
}

bool EventLoop::run(double timeout_s) {
    TimePoint deadline = timeAfter(timeout_s);
    while (true) {
        // First run timers, regardless of deadline:
        while (Task *task = nextTimerToExecute()) {
            double ts = task->timerEvent();
            if (ts > 0) {
                auto t = std::chrono::microseconds(toUs(ts));
                timer_queue.insert(std::make_pair(timeNow()+t, task));
            }
        }
        check_finished();

#ifdef USE_THREADS
        if ( (tasks.empty() && threads.empty()) ||
            timeNow() > deadline || aborted())
#else
        if (tasks.empty() || timeNow() > deadline || aborted())
#endif
            break;

        double time_left;
        if (timer_queue.empty()) {
            time_left = secondsTo(deadline);
        } else {
            auto next_timer = timer_queue.cbegin()->first;
            time_left = secondsTo(std::min(next_timer, deadline));
        }

        if (time_left <= 0) {
            log() << "Timeout passed, will not poll connections";
            break;
        }
        if (!engine.run(time_left)) {
            err_log() << "fatal engine failure";
            do_abort = true;
            break;
        }
        check_finished();
    }
    if (do_abort)
        removeAllTasks();
#ifdef USE_THREADS
    return (!tasks.empty() || !threads.empty());
#else
    return (!tasks.empty());
#endif
}

void EventLoop::runUntilComplete() {
    while (run(1.5)) {
        // for (auto &p : tasks)
        //     log() << "REM: " << p.first->label();
    }
#ifdef USE_THREADS
    dbg_log() << "End Loop; Threads left: " << threads.size();
    waitForThreadsToFinish();
#endif
    flushLogFile();
}

#ifdef USE_THREADS
void EventLoop::runTask(Task *task, const std::string &name,
                        std::ostream *log_file, EventLoop *parent) {
#else
void EventLoop::runTask(Task *task, const std::string &name,
                        std::ostream *log_file) {
#endif
    if (log_file)
        setLogFile(*log_file);
#ifdef USE_THREADS
    if (task->is_child_thread)
        parent = nullptr;
    EventLoop loop(name, parent);
#else
    EventLoop loop(name);
#endif
    loop.addTask(task);
    loop.runUntilComplete();
    flushLogFile();
}

#ifdef USE_THREADS

void EventLoop::spawnThread(Task *task, const std::string &name,
                            std::ostream *log_file, Task *parent) {
    if (parent_loop)
        throw std::runtime_error("not in main loop");
    if (parent)
        threadTaskObserver.insert(std::make_pair(task, parent));
    task->is_child_thread = true;

    threads.insert(std::make_pair(task, std::thread(runTask, task, name,
                                                    log_file, this)));
    dbg_log() << "Added thread, now have " << threads.size();
}

MsgQueue<Task *>  EventLoop::finished_threads;

void EventLoop::collect_thread(Task *t) {
    if (parent_loop)
        return;
    log() << "Thread task " << t->label() << " finished";
    auto p = threads.find(t);
    if (p != threads.end()) {
        p->second.join();
        threads.erase(p);
    }

    // Notify parent?
    auto it = threadTaskObserver.find(t);

    // TODO: make sure to erase (task, parent) from
    // threadTaskObserver if parent dies first
    if (it != threadTaskObserver.end()) {
        Task *parent = it->second;
        if (tasks.find(parent) != tasks.end())
            parent->taskFinished(t);
        threadTaskObserver.erase(it);
    }

    delete t;
}

void EventLoop::waitForThreadsToFinish() {
    while (!threads.empty()) {
        dbg_log() << "Threads left: " << threads.size();
        Task *t = finished_threads.pop_blocking();
        collect_thread(t);
    }
}

#endif

void EventLoop::removeAllTasks() {
    while (!tasks.empty())
        _removeTask(tasks.begin()->first, true);
}

void EventLoop::check_finished() {
    if (got_signal) {
        int signum = got_signal;
        auto p = userSignalHandler.lower_bound(signum);
        auto to = userSignalHandler.upper_bound(signum);
        if (p == to) {
            // No user defined handler for signal, will abort
#ifdef _WIN32
            err_log() << "got signal " << signum << ", will exit";
            abort();
#else
            if (signum == SIGPIPE) {
                warn_log() << "got SIGPIPE";
            } else if (signum == SIGHUP) {
                Logger::reopenLogFile(openFileOnSIGHUP);
                Logger::setLogLimit();
                log() << "got SIGHUP";
                killChildProcesses(signum);
            } else if (signum == SIGTERM) {
                err_log() << "got SIGTERM, exit immediately" << std::endl;
                killChildProcesses(signum);
                exit(1);
            } else {
                err_log() << "got signal " << signum << ", will exit";
                killChildProcesses(signum);
                abort();
            }
#endif
        } else {
            for (; p != to; ++p) {
                void (*handler)(int, EventLoop &) = p->second;
                (*handler)(got_signal, *this);
            }
        }
        got_signal = 0;
    }
#ifndef _WIN32
    size_t i = 0;
    while (terminatedPIDtmp[i] && i<sizeof(terminatedPIDtmp)/sizeof(int)) {
        int pid = terminatedPIDtmp[i], wstatus = terminatedPIDtmp[i+1];
        log() << "PID " << pid << " finished, status=" << wstatus;
        terminatedPIDs[pid] = wstatus;
        terminatedPIDtmp[i] = 0;
        i += 2;
    }
    while (!terminatedPIDs.empty()) {
        auto it = terminatedPIDs.begin();
        int pid = it->first;
        int wstatus = it->second;
        log() << "Terminated PID " << pid << " status " << wstatus;
        terminatedPIDs.erase(it);
        auto it2 = pidOwner.find(pid);
        if (it2 != pidOwner.end()) {
            Task *task = it2->second;
            if (tasks.find(task) != tasks.end()) {
                task->processFinished(pid, wstatus);
            }
            pidOwner.erase(it2);
        }
    }
#endif
#ifdef USE_THREADS
    Task *t;
    while (!threads.empty() && finished_threads.fetch(t))
        collect_thread(t);
#endif
    while (!messageTasks.empty() || !finishedTasks.empty()) {
        if (!messageTasks.empty()) {
            auto p = tasks.find(*messageTasks.begin());
            messageTasks.erase(messageTasks.begin());
            if (p != tasks.end()) {
                if (Task *parent = p->second)
                    parent->taskMessage(p->first);
            }
        }
        if (!finishedTasks.empty()) {
            auto p = tasks.find(*finishedTasks.begin());
            finishedTasks.erase(finishedTasks.begin());
            if (p != tasks.end())
                _removeTask(p->first);
        }
    }
}
