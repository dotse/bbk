// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

#include <queue>
#include <vector>
#include <map>
#include <set>
#include <functional>

#include "httpclienttask.h"

struct HttpRequestEngineEvent {
    const std::string &url;
    unsigned int http_status;
    const std::string &response;
};

class HttpRequestEngine : public HttpClientTask {
public:
    HttpRequestEngine(const std::string &name, const HttpHost &host,
                      unsigned int min_conn = 0, unsigned int max_conn = 10,
                      double tick_duration = 0.5) :
        HttpClientTask(name, host),
        min_connections(std::min(min_conn, max_conn)),
        max_connections(max_conn),
        no_connections(min_connections),
        active_connections(0),
        tick(tick_duration > 0.0 ? tick_duration : 0.5) {
    }

    // Will call task's handleExecution when done, if task has told me to
    // observe it. I.e. task must have done engine->startObserving(this);
    // Also, if event_name is an empty string, handleExecution won't be called.
    void getJob(Task *task, const std::string &event_name,
                const std::string &url) {
        postJob(task, event_name, url, "");
    }

    void postJob(Task *task, const std::string &event_name,
                 const std::string &url, const std::string &data);

    // Return HTTP status of last completed request, 0 for failure.
    // Should only be called in the handleExecution callback.
    unsigned int httpStatus() const {
        return last_completed ? last_completed->httpStatus() : 0;
    }

    // Return MIME type of last completed request, empty on failure.
    // Should only be called in the handleExecution callback.
    std::string contentType() const {
        return last_completed ? last_completed->contentType() : "";
    }

    // Return the payload of the last completed request, empty on failure.
    // Should only be called in the handleExecution callback.
    const std::string contents() const {
        return last_completed ? last_completed->contents() : "";
    }

    // Return the URL of the last completed request, empty on failure.
    // Should only be called in the handleExecution callback.
    const std::string &currentUrl() const {
        static std::string dummy;
        return current_job ? current_job->url : dummy;
    }

    // Perform the same request again.
    // Should only be called in the handleExecution callback.
    void redoJob() {
        redo_job = current_job;
    }


    // Perform another GET request, but with updated url.
    void redoJob(const std::string &url) {
        if (current_job) {
            current_job->url = url;
            current_job->data.clear();
        }
    }

    // Perform another POST request, but with updated url/data.
    void redoJob(const std::string &url, const std::string &data) {
        if (current_job) {
            current_job->url = url;
            current_job->data = data;
        }
    }

    // If a task terminates, release ongoing jobs
    void taskFinished(Task *t) override;

    // Will be called if all connections have failed.
    // Default is to try again after next tick.
    virtual void connectionLost() {
        dbg_log() << "connectionLost() not implemented";
    }
private:
    struct HREJob {
        Task *task;
        HttpClientConnection *connection;
        std::string event_name, url, data;
    };

    double start() final;

    double timerEvent() final;

    // Cancel all request owned by task. Handlers won't be executed.
    void cancelRequestsByTask(Task *task);

    void newRequest(HttpClientConnection *conn) final;
    bool requestComplete(HttpClientConnection *conn) final;
    void connAdded(SocketConnection *) final;
    void connRemoved(SocketConnection *) final;
    void checkConnectionCount();
    void checkQueue();

    unsigned int min_connections, max_connections;
    unsigned int no_connections, active_connections;
    double tick;

    HttpClientConnection *last_completed = nullptr;
    HREJob *current_job;

    HREJob *redo_job;

    //size_t max_response_size = 10000000;

    // Jobs waiting to be started
    std::deque<HREJob *> incoming_jobs;

    std::set<SocketConnection *> idle_connections;
    std::map<HttpClientConnection *, HREJob *>  active_jobs;
};
