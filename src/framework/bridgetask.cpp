// Copyright (c) 2019 Internetstiftelsen
// Written by Göran Andersson <initgoran@gmail.com>

#include "../json11/json11.hpp"

#include "bridgetask.h"

BridgeTask::~BridgeTask() {
}

double BridgeTask::start() {
    if (the_agent) {
        addNewTask(the_agent, this);
        the_agent->startObserving(this);
    } else {
        err_log() << "You must call setAgent before starting bridge";
        setError("no agent");
    }
    return 0;
}
