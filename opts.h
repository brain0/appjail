#pragma once

typedef enum { false, true } bool;

typedef struct {
  bool allow_new_privs;
  char **argv;
} appjail_options;

void parse_options(appjail_options *opts, int argc, char *argv[]);
