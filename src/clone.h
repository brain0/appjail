#pragma once

#include <signal.h>
#include <unistd.h>

typedef struct {
  int sfd;
  sigset_t *old_sigmask;
} child_options;

pid_t launch_child(int flags, child_options *chldopts, int (*fn)(void *), void *arg);
