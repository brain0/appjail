#pragma once

#include "common.h"
#include <sys/capability.h>
#include <unistd.h>

bool want_cap(cap_value_t c);
void need_cap(cap_value_t c);
void drop_caps();
void drop_caps_forever();
int cap_mount(const char *source, const char *target,
              const char *filesystemtype, unsigned long mountflags,
              const void *data);
int cap_umount2(const char *target, int flags);
int cap_chown(const char *path, uid_t owner, gid_t group);
