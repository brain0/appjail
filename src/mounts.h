#pragma once
#include "opts.h"

void init_libmount();
void set_mount_propagation_slave();
void sanitize_mounts(appjail_options *opts);
void unmount_directory(const char *path);
