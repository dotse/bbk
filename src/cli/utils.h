// Copyright (c) 2018 IIS (The Internet Foundation in Sweden)
// Written by Göran Andersson <initgoran@gmail.com>

#pragma once

class TaskConfig;

std::string createAndGetAppDir();

bool parseArgs(int argc, char *argv[],
               TaskConfig &client_cfg, TaskConfig &agent_cfg);
