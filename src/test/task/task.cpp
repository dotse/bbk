#include <gtest/gtest.h>

#include <framework/eventloop.h>
#include <framework/task.h>

namespace {

    std::string global_string;

    class BbkEnvironment : public ::testing::Environment {
    public:
	BbkEnvironment() : global_log("log.txt") {
	}

	void SetUp() override {
	    Logger::setLogFile(global_log);
	}
    private:
	std::ofstream global_log;
    };

    testing::Environment* const foo_env =
	testing::AddGlobalTestEnvironment(new BbkEnvironment);

    class Task1 : public Task {
    public:
	Task1(const std::string &name, const std::string &secret) :
	    Task(name),
	    start_secret(secret) {
	}
	double start() override {
	    global_string = start_secret;
	    setResult("Task1 done");
	    return 0.0;
	}
    private:
	std::string start_secret;
    };

    class Task2 : public Task {
    public:
	Task2(const std::string &secret) :
	    Task("Task2"),
	    timer_secret(secret) {
	}
	double start() override {
	    return 1.0;
	}
	double timerEvent() override {
	    global_string = timer_secret;
	    setResult("Task2 done");
	    return 0.0;
	}
    private:
	std::string timer_secret;
    };

}

TEST(TaskTest, start1) {
    const std::string taskname = "test-start1";
    const std::string secret = "secret-start1";
    Task *t = new Task1(taskname, secret);
    ASSERT_EQ(t->label(), taskname);
    global_string.clear();
    EventLoop::runTask(t);
    ASSERT_EQ(global_string, secret);
}

TEST(TaskTest, timer1) {
    const std::string secret = "secret-timer1";
    Task *t = new Task2(secret);
    global_string.clear();
    EventLoop::runTask(t);
    ASSERT_EQ(global_string, secret);
}
