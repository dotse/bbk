#include "httprequestengine.h"

void HttpRequestEngine::postJob(Task *task, const std::string &event_name,
                                const std::string &url,
                                const std::string &data) {
    auto job = new HREJob({ task, nullptr, event_name, url, data });
    incoming_jobs.push_back(job);

    dbg_log() << "JOB " << url << " Q=" << incoming_jobs.size()
              << " I=" << idle_connections.size()
              << " C=" << no_connections;
    checkQueue();
}

void HttpRequestEngine::checkQueue() {
    if (incoming_jobs.empty())
        return;

    while (!idle_connections.empty()) {
        auto p = idle_connections.begin();
        SocketConnection *s = *p;
        idle_connections.erase(p);
        if (wakeUpConnection(s))
            return;
    }

    dbg_log() << "No idle connection available, have " << no_connections
              << " max=" << max_connections;

    if (no_connections < max_connections) {
        // Increase number of simultaneous connections
        ++no_connections;
        checkConnectionCount();
    }

    // We'll have to wait for an existing connection to become available.
}

double HttpRequestEngine::start() {
    log() << "start()";
    if (incoming_jobs.size() > min_connections) {
        if (incoming_jobs.size() > max_connections)
            no_connections = max_connections;
        else
            no_connections = static_cast<unsigned int>(incoming_jobs.size());
    }
    checkConnectionCount();
    return tick;
}

double HttpRequestEngine::timerEvent() {
    if (no_connections > min_connections) {
        unsigned int n = static_cast<unsigned int>(active_jobs.size() +
                                                   incoming_jobs.size());
        // Will only need n simultaneous connections
        if (n < no_connections)
            no_connections = std::max(n, min_connections);
    }

    // There may be failed jobs waiting to be restarted:
    checkQueue();

    return tick;
}

void HttpRequestEngine::newRequest(HttpClientConnection *conn) {
    dbg_log() << "Ready for new request, queue size=" << incoming_jobs.size();

    auto p = active_jobs.find(conn);
    if (p != active_jobs.end()) {
        HREJob *job = p->second;
        if (job->data.empty())
            conn->get(job->url);
        else
            conn->post(job->url, job->data);
        return;
    }

    while (!incoming_jobs.empty()) {
        HREJob *job = incoming_jobs.front();
        incoming_jobs.pop_front();
        if (!job)
            continue;
        log() << "Job " << job->event_name << ": " << job->url;
        job->connection = conn;
        if (job->data.empty())
            conn->get(job->url);
        else
            conn->post(job->url, job->data);
        active_jobs[conn] = job;
        return;
    }

    //dbg_log() << "nothing to do";
    idle_connections.insert(conn);
    conn->pass();
}

bool HttpRequestEngine::requestComplete(HttpClientConnection *conn) {
    dbg_log() << "HttpRequestEngine::requestComplete " << conn->httpStatus()
              << " --> " << conn->contents();

    {
        auto p = active_jobs.find(conn);
        if (p == active_jobs.end())
            return true;

        last_completed = conn;
        current_job = p->second;
    }

    HREJob *job = current_job;
    redo_job = nullptr;

    if (!current_job->event_name.empty())
        executeHandler(current_job->task, current_job->event_name);
    if (redo_job == job) {
        // User has asked for a new request, let conn stay in active_jobs.
        redo_job = nullptr;
    } else {
        delete job;
        active_jobs.erase(conn);
    }
    last_completed = nullptr;
    return true;
}

void HttpRequestEngine::taskFinished(Task *task) {
    dbg_log() << "Client task " << task->label() << " died.";
    cancelRequestsByTask(task);
}

void HttpRequestEngine::cancelRequestsByTask(Task *task) {

    // Cancel active jobs
    dbg_log() << "Cancel active jobs";

    std::set<HttpClientConnection *> to_remove;
    for (auto p : active_jobs)
        if (p.second->task == task) {
            // Clear event_name to stop handler from being executed:
            p.second->event_name.clear();
            to_remove.insert(p.first);
        }

    for (auto &conn : to_remove)
        cancelConnection(conn);

    dbg_log() << "Cancel pending jobs";

    // Cancel pending jobs
    for (auto &job : incoming_jobs) {
        if (job && job->task == task) {
            delete job;
            job = nullptr;
        }
    }
}

void HttpRequestEngine::connAdded(SocketConnection *c) {
    ++active_connections;
    dbg_log() << "conn added: " << c << " now have " << active_connections;
}

void HttpRequestEngine::connRemoved(SocketConnection *c) {
    --active_connections;
    dbg_log() << "conn removed: " << c << " now have " << active_connections;

    if (idle_connections.erase(c)) {
        // An idle connection was closed, perhaps due to keep-alive timeout.
        if (no_connections > min_connections)
            --no_connections; // Don't restart the connection now.
        return;
    }

    HttpClientConnection *conn = dynamic_cast<HttpClientConnection *>(c);
    auto p = active_jobs.find(conn);
    if (p == active_jobs.end())
        return;

    HREJob *job = p->second;
    active_jobs.erase(p);
    if (job->event_name.empty())
        return;

    // Notify user that request failed
    last_completed = nullptr;
    redo_job = nullptr;
    current_job = job;
    executeHandler(current_job->task, current_job->event_name);
    if (redo_job == job) {
        redo_job = nullptr;
        // Probably better to wait a little while before restarting.
        // Put it last in queue, and don't do checkQueue(); now.
        incoming_jobs.push_back(job);
    } else {
        delete current_job;
    }
}

void HttpRequestEngine::checkConnectionCount() {
    if (!hasStarted() || terminated()) {
        dbg_log() << "FAIL! HttpRequestEngine not active!";
        return;
    }

    dbg_log() << "checkConnectionCount: act=" << active_connections
              << " want=" << no_connections
              << " max=" << max_connections
              << " Q=" << incoming_jobs.size()
              << " I=" << idle_connections.size();

    for (unsigned int i = active_connections; i < no_connections; ++i) {
        if (!createNewConnection())
            log() << "couldn't add connection";
    }

    if (no_connections && !active_connections)
        connectionLost();
}
