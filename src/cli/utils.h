// Copyright (c) 2018 The Swedish Internet Foundation
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

class TaskConfig;

std::string createAndGetAppDir(std::string dir = "");

bool parseArgs(int argc, char *argv[],
               TaskConfig &client_cfg, TaskConfig &agent_cfg);
