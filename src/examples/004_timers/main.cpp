#include <framework/task.h>
#include <framework/eventloop.h>

/* Example 004

Simple demonstration of tasks and timers.
Write log to stderr.

*/

class PointlessTask : public Task {
public:

    PointlessTask(const std::string &name,
                  double tick_length, unsigned int no_ticks) :
        Task(name),
        tick_duration(tick_length),
        ticks(no_ticks), curr_tick(0) {
    }

    double start() override {
        dbg_log() << "starting, " << ticks << " ticks";
        return tick_duration;
    }

    double timerEvent() override {
        log() << "timerEvent " << ++curr_tick << " of " << ticks;
        if (curr_tick < ticks)
            return tick_duration;
        setResult("Done after " + std::to_string(elapsed()) + " seconds");
        return 0.0;
    }
private:
    double tick_duration;
    unsigned int ticks, curr_tick;
};

int main(int , char *[]) {
    EventLoop loop;
    loop.addTask(new PointlessTask("Pointless 1", 1.0, 3));
    loop.addTask(new PointlessTask("Pointless 2", 0.7, 5));
    loop.addTask(new PointlessTask("Pointless 3", 0.5, 7));
    loop.runUntilComplete();
    return 0;
}
