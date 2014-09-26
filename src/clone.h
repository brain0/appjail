#pragma once

#include <unistd.h>

pid_t my_clone(int flags, int (*fn)(void *), void *arg);
