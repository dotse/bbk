// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

// This class manages Task objects and their timers. It also manages
// all network connections through an Engine object. The Engine is an
// "inner event loop" used to manage low-level network events.
//
// Your code must create an EventLoop object, add one or more Task objects
// to it, and then run the event loop, either "forever" using the method
//     runUntilComplete()
// or by regularly calling the method
//     run(double timeout_s)
//

#pragma once

#include <set>
#include <map>

#include "logger.h"
#include "engine.h"
#ifdef USE_THREADS
#include "msgqueue.h"
#endif

class Task;
class SocketConnection;
class ServerSocket;
class WorkerProcess;

class EventLoop : public Logger {
public:
    EventLoop(std::string log_label = "MainLoop") :
        Logger(log_label),
        engine("NetworkEngine"),
        name(log_label) {
#ifdef USE_THREADS
        do_init(nullptr);
#else
        do_init();
#endif
    }

    void removeAllTasks();

    ~EventLoop();

    // Add a task to be managed by the EventLoop. The value of the task
    // parameter must be an object of a subclass of Task. You _must_ create
    // the object with new; it cannot be an object on the stack.
    // The EventLoop will take ownership of the object and will delete it
    // when the task has finished. Before it is deleted, the parent's
    // taskFinished method will be called unless the parent is nullptr.
    void addTask(Task *task, Task *parent = nullptr);

    // Run for at most timeout_s seconds.
    // Returns false if all done, otherwise true:
    bool run(double timeout_s);

    // Run until all task are done.
    void runUntilComplete();

#ifdef USE_THREADS
    // Creates an EventLoop object that runs the task until it's finished.
    // You cannot use this if you have created your own EventLoop object.

    // The "parent" parameter is used if the main thread (the parent) should
    // be notified when the thread running the task is finished.
    static void runTask(Task *task, const std::string &name = "MainLoop",
                        std::ostream *log_file = nullptr,
                        EventLoop *parent = nullptr);
#else
    static void runTask(Task *task, const std::string &name = "MainLoop",
                        std::ostream *log_file = nullptr);
#endif

    void waitForThreadsToFinish();

    // Remove the given task
    void abortTask(Task *task);

    // Remove all tasks
    void abort() {
        interrupt();
        do_abort = true;
    }

    // Get all tasks ith the given parent
    void getChildTasks(std::set<Task *> &tset, Task *parent) const;

    void abortChildTasks(Task *parent);

    // Restart idle connections:
    void wakeUpTask(Task *t);

    bool wakeUpConnection(SocketConnection *s) {
        return engine.wakeUpConnection(s);
    }

    void cancelConnection(SocketConnection *s) {
        engine.cancelConnection(s);
    }

    std::set<Socket *> findConnByTask(const Task *t) const;

    // Return true if conn still exists:
    bool isActive(const Socket *conn) const {
        return engine.connActive(conn);
    }

    // Remove previous timer, run after s seconds instead.
    // If s = 0, run timer immediately. If s < 0, remove timer.
    void resetTimer(Task *task, double s);

    // Create a new socket connection, and add it to the loop.
    // Returns false (and deletes conn) on failure.
    // On success, returns true and calls connAdded on owner task.
    // A connection to the server will be initiated. When connected, the
    // connected() method will be called on conn to get initial state.
    bool addConnection(SocketConnection *conn);

    // Use this if conn contains a socket that has already been connected.
    // Returns false (and deletes conn) on failure.
    // On success, returns true and calls connAdded on owner task,
    // then calls connected() on conn to get initial state.
    bool addConnected(SocketConnection *conn);

    // Returns false (and deletes conn) on failure.
    // On success, returns true and calls serverAdded on owner task.
    bool addServer(ServerSocket *conn);
#ifdef USE_GNUTLS
    bool tlsSetKey(ServerSocket *conn, const std::string &crt_path,
                   const std::string &key_path, const std::string &password) {
        return engine.tlsSetKey(conn, crt_path, key_path, password);
    }
    bool setCABundle(const std::string &path) {
        return engine.setCABundle(path);
    }
#endif

    // Returns true if task is running
    bool running(Task *task);

    void taskDeleted(Task *task);

    // Call this to make the network engine yield control
    // to me (the task supervisor):
    static void interrupt() {
        Engine::yield();
    }

#ifdef USE_THREADS
    // Create a new thread and run task in its own loop in that thread.
    void spawnThread(Task *task, const std::string &name="ThreadLoop",
                     std::ostream *log_file = nullptr,
                     Task *parent = nullptr);
    void waitForThreadsToComplete();
#endif
#ifdef _WIN32
    int externalCommand(Task *owner, const char *const argv[]) {
        // Not implemented
        exit(1);
    }
#else
    // Asynchronously execute an external command.
    // Return false on immediate failure.
    int externalCommand(Task *owner, const char *const argv[]);
    static void daemonize();

    // Create a child process. Return child's PID. Channels can be
    // used to pass sockets and messages between parent and child.
    WorkerProcess *createWorker(Task *parent, std::ostream *log_file,
                                unsigned int channels, unsigned int wno);

    WorkerProcess *createWorker(Task *parent, const std::string &log_file_name,
                                unsigned int channels, unsigned int wno);

    // Send signal to all child processes
    void killChildProcesses(int signum);

    static void setLogFilename(const std::string &filename) {
        openFileOnSIGHUP = filename;
    }
#endif

    void notifyTaskFinished(Task *task) {
        auto ret = finishedTasks.insert(task);
        if (ret.second)
            engine.yield();
    }
    void notifyTaskMessage(Task *task) {
        auto ret = messageTasks.insert(task);
        if (ret.second)
            engine.yield();
    }

    bool aborted() const {
        return do_abort;
    }

    void addSignalHandler(int signum, void (*handler)(int, EventLoop &));
    static void signalHandler(int signum);

    // Enable events from task "from" to task "to". I.e. "from" will be able to
    // call executeHandler with "to" as a parameter. If  "to" dies before
    // "from", "from" will be notified through a call to taskFinished.
    // Will return false unless both tasks still exist.
    bool startObserving(Task *from, Task *to);

    // Return true if observer is observing task.
    bool isObserving(Task *observer, Task *task) const {
        auto p = observed_by.find(observer);
        return p != observed_by.end() &&
            p->second.find(task) != p->second.end();
    }

private:
#ifndef _WIN32
#ifdef USE_THREADS
    thread_local
#endif
    static std::map<int, int> terminatedPIDs;
    std::map<int, Task *> pidOwner;
#ifdef USE_THREADS
    thread_local
#endif
    static std::string openFileOnSIGHUP;
#endif
#ifdef USE_THREADS
    void do_init(EventLoop *parent);

    EventLoop(std::string log_label, EventLoop *parent) :
        Logger(log_label),
        engine("NetworkEngine"),
        name(log_label) {
        do_init(parent);
    }
    std::map<Task *, std::thread> threads;
    static MsgQueue<Task *> finished_threads;
    std::map<Task *, Task *> threadTaskObserver;
    void collect_thread(Task *t);
    EventLoop *parent_loop;
#else
    void do_init();
#endif

#ifdef USE_THREADS
    //thread_local
#endif
    static volatile int got_signal;
#ifdef USE_THREADS
    thread_local
#endif
    static volatile int terminatedPIDtmp[100];

    void check_finished();
    Task *nextTimerToExecute();
    void _removeTimer(Task *task);
    void _removeTask(Task *task, bool killed = false);
    Engine engine;

    // Map each task to its parent (or, if it has no parent, to nullptr)
    std::map<Task *, Task *> tasks;

    // These are used to keep track of "observation" between tasks.
    // E.g. a parent task is observing its children.
    std::map<Task *, std::set<Task *> > observed_by;
    std::map<Task *, std::set<Task *> > observing;

    std::set<Task *> finishedTasks, messageTasks;
    std::multimap<int, void (*)(int, EventLoop &)> userSignalHandler;
    std::multimap<TimePoint, Task *> timer_queue;
    std::string name;
    bool do_abort = false;
};
