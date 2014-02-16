#pragma once

typedef enum { false, true } bool;

typedef struct {
  bool allow_new_privs;
  const char *homedir;
  char **argv;
} appjail_options;

void parse_options(appjail_options *opts, int argc, char *argv[]);
