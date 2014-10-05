#pragma once

#include <unistd.h>

void wait_for_child(pid_t pid1, int sfd, int pipefd);
