#include "singlerequest.h"

double SingleRequest::start() {
    if (!createNewConnection())
        setError("SingleRequest Failure");
    return _timeout;
}

double SingleRequest::timerEvent() {
    if (!terminated())
        setError("SingleRequest Timeout");
    return 0.0;
}

bool SingleRequest::requestComplete(HttpClientConnection *conn) {
    setResult(conn->contents());
    _status = conn->httpStatus();
    return false;
}
