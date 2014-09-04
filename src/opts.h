#pragma once

#include "common.h"

typedef struct {

  bool allow_new_privs;
  bool keep_shm;
  const char *homedir;

  char **argv;

  char **unmount_directories;
  char **shared_directories;
  bool keep_x11;
} appjail_options;

appjail_options *parse_options(int argc, char *argv[]);
void free_options(appjail_options *opts);
