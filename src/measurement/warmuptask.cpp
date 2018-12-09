#include "warmuptask.h"

void WarmUpTask::newRequest(HttpClientConnection *conn) {
    conn->get(url);
}

bool WarmUpTask::requestComplete(HttpClientConnection *) {
    return false;
}
