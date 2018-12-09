#include "progresstask.h"

void ProgressTask::doTestProgress(double mbps, double duration,
                                  unsigned int no_conn) {
    if (duration <= current_duration || terminated())
        return;

    current_duration = duration;

    // Don't let the speed decrease the last half second:
    if (duration < tot_duration-0.5 || mbps > current_mbps)
        current_mbps = mbps;

    if (duration >= tot_duration) {
        setResult(fValue(current_mbps));
        return;
    }

    setMessage("progress");
    log() << "task progress " << current_mbps << ' ' << duration/tot_duration;

    if (duration > tot_duration-0.35) {
        // Less than 0.35 seconds left, don't make any new requests
        // (but let existing requests keep going).
        noMoreConnections();
        current_load_size = 0;
        soon_finished = true;
        return;
    }

    // Calculate appropriate load size for subsequent requests

    //unsigned int n = getNoConnections();
    if (!no_conn)
        return;

    double time_left = tot_duration - duration;

    // We'll probably load this number of bytes during the time_left:
    double exp_bytes;
    if (speedlimit_mbps > 0.0)
        exp_bytes = std::min(speedlimit_mbps, mbps) * std::min(time_left, 0.3) / 0.000008;
    else
        exp_bytes = mbps * time_left / 0.000008;

    current_load_size = static_cast<size_t>(exp_bytes / 4.0 / no_conn);
    if (current_load_size > 40000000)
        current_load_size = 40000000;
    else if (current_load_size < 6000)
        current_load_size = 6000;

    if (speedlimit_mbps > mbps) {
        dbg_log() << "We're going too slow, wake up the passive connections";
        wakeUp();
    }
}

size_t ProgressTask::loadSize() {
    // For very fast network, speed up even before first doTestProgress:
    if (++no_started_loads == load_size_check && current_duration<=0) {
        double time = elapsed();
        double speed = addOverheadMbps(byteCount(), time);
        doTestProgress(speed, time, currentNoConnections());
    }
    if (speedlimit_mbps > 0.0) {
        double time = elapsed();
        double speed = addOverheadMbps(byteCount(), time);
        if (speed > speedlimit_mbps) {
            log() << "going too fast, will pause";
            return 0;
        }
    }

    return current_load_size;
}

bool ProgressTask::requestComplete(HttpClientConnection *) {
    return true;
}
