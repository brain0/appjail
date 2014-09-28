#pragma once

#define _GNU_SOURCE

#include "config.h"
#include <stdio.h>
#include <stdlib.h>

typedef enum { false, true } bool;

typedef enum { RUN_HOST, RUN_USER, RUN_PRIVATE } run_mode_t;

#define errExit(msg) do { \
  perror(msg);\
  exit(EXIT_FAILURE);\
  } while (0)

#define errExitNoErrno(err) do { \
  fprintf(stderr, err "\n");\
  exit(EXIT_FAILURE);\
  } while (0)

#define errWarn(msg) do { \
  perror(msg);\
  } while (0)

bool string_to_unsigned_integer(unsigned int *val, const char *str);
bool string_to_integer(int *val, const char *str);
