#pragma once

#include "common.h"
#include "list.h"

void setup_environment(char ***envp, bool cleanenv, strlist *keep_env, strlist *set_env);
