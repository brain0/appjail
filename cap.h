#pragma once

#include <sys/capability.h>

void need_cap(cap_value_t c);
void drop_caps();
void drop_caps_forever();
int cap_mount(const char *source, const char *target,
              const char *filesystemtype, unsigned long mountflags,
              const void *data);
int cap_umount2(const char *target, int flags);
