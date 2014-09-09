#pragma once

#include "common.h"
#include "configfile.h"

typedef struct {
  uid_t uid;
  char *user;

  bool allow_new_privs;
  bool keep_shm;
  const char *homedir;
  run_mode_t run_mode;
  bool bind_run_media;

  char **argv;

  char **keep_mounts, **keep_mounts_full;
  char **shared_mounts;
  char **special_mounts;
  bool keep_x11;
  bool unshare_network;
} appjail_options;

appjail_options *parse_options(int argc, char *argv[], appjail_config *config);
void free_options(appjail_options *opts);

bool string_to_run_mode(run_mode_t *result, const char *s);
