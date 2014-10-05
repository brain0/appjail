#pragma once

#include "common.h"
#include <unistd.h>

void wait_for_child(pid_t pid1, int sfd, bool daemonize, int pipefd);
