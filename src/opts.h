#pragma once

typedef enum { false, true } bool;

typedef struct {

  bool allow_new_privs;
  const char *homedir;

  char **argv;

  char **unmount_directories;
  char **shared_directories;
} appjail_options;

appjail_options *parse_options(int argc, char *argv[]);
void free_options(appjail_options *opts);
