#pragma once

#define _GNU_SOURCE

#include "config.h"
#include <stdio.h>
#include <stdlib.h>

#define errExit(msg) do { \
  perror(msg);\
  exit(EXIT_FAILURE);\
  } while (0)

#define errExitNoErrno(err) do { \
  fprintf(stderr, err "\n");\
  exit(EXIT_FAILURE);\
  } while (0)
