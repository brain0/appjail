#pragma once

#include "common.h"
#include "configfile.h"

typedef struct {

  bool allow_new_privs;
  bool keep_shm;
  const char *homedir;

  char **argv;

  char **keep_mounts, **keep_mounts_full;
  char **shared_mounts;
  char **special_mounts;
  bool keep_x11;
  bool unshare_network;
} appjail_options;

appjail_options *parse_options(int argc, char *argv[], appjail_config *config);
void free_options(appjail_options *opts);
