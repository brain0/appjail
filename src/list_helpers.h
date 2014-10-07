#pragma once

#include "common.h"
#include "list.h"

typedef enum {
  HAS_CHILD_OF_NEEDLE,
  HAS_PARENT_OF_NEEDLE,
  HAS_STRICT_PARENT_OF_NEEDLE,
  HAS_EXACT_PATH
} has_path_mode_t;

bool has_path(strlist *l, const char *needle, has_path_mode_t mode);
bool strlist_contains(strlist *l, char *s);
size_t strlist_count(strlist *l);
bool intlist_contains(intlist *l, int i);
