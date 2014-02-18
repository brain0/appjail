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

appjail_options *parse_options(int argc, char *argv[]);
void free_options(appjail_options *opts);
