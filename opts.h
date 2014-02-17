#pragma once

typedef enum { false, true } bool;

typedef struct {
  bool allow_new_privs;
  const char *homedir;
  char **argv;
  struct {
    const char **unmount_directories, **shared_directories;
  } special_directories;
} appjail_options;

void parse_options(appjail_options *opts, int argc, char *argv[]);
