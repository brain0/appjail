#pragma once

#include "common.h"

typedef struct {
  bool allow_new_privs_permitted;
} appjail_config;

appjail_config *parse_config();
void free_config(appjail_config *config);
