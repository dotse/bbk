#include "../json11/json11.hpp"

#include "bridgetask.h"

BridgeTask::~BridgeTask() {
}

double BridgeTask::start() {
    addNewTask(the_agent, this);
    the_agent->startObserving(this);
    return 0;
}
