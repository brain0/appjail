#pragma once

#include "common.h"
#include <signal.h>
#include <unistd.h>

typedef struct {
  int sfd;
  sigset_t *old_sigmask;
  bool daemonize;
} child_options;

pid_t launch_child(int flags, child_options *chldopts, int (*fn)(void *), void *arg);
