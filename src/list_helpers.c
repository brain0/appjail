#include "list_helpers.h"
#include <string.h>

static bool s_is_child_of_n(const char *s, size_t len_s, const char *n, size_t len_n, bool strict) {
  return !strncmp(s, n, len_n) && ((!strict && len_n == len_s) || (len_s > len_n && s[len_n] == '/'));
}

static bool test_s_is_child_of_n(const char *s, size_t len_s, const char *n, size_t len_n) {
  return s_is_child_of_n(s, len_s, n, len_n, false);
}

static bool test_s_is_parent_of_n(const char *s, size_t len_s, const char *n, size_t len_n)  {
  return s_is_child_of_n(n, len_n, s, len_s, false);
}

static bool test_s_is_strict_parent_of_n(const char *s, size_t len_s, const char *n, size_t len_n)  {
  return s_is_child_of_n(n, len_n, s, len_s, true);
}

static bool test_exact_match(const char *s, size_t len_s, const char *n, size_t len_n) {
  return !strcmp(s, n);
}

bool has_path(strlist *l, const char *needle, has_path_mode_t mode) {
  strlist_node *i;
  size_t len, len_needle;
  const char *s;
  bool (*test_fn)(const char *s, size_t len_s, const char *n, size_t len_n);

  switch(mode) {
    case HAS_CHILD_OF_NEEDLE:
      test_fn = test_s_is_child_of_n;
      break;
    case HAS_PARENT_OF_NEEDLE:
      test_fn = test_s_is_parent_of_n;
      break;
    case HAS_STRICT_PARENT_OF_NEEDLE:
      test_fn = test_s_is_strict_parent_of_n;
      break;
    case HAS_EXACT_PATH:
      test_fn = test_exact_match;
      break;
    default:
      return false;
  }

  len_needle = strlen(needle);
  /* Deal with trailing slashes */
  if(needle[len_needle-1] == '/')
    len_needle--;
  for(i = strlist_first(l); i != NULL; i = strlist_next(i)) {
    /* We assume that the list entries never have
     * trailing slashes.
     */
    s = strlist_val(i);
    len = strlen(s);
    if(test_fn(s, len, needle, len_needle))
      return true;
  }

  return false;
}

size_t strlist_count(strlist *l) {
  strlist_node *n;
  size_t i;

  for(n = strlist_first(l), i = 0; n != NULL; n = strlist_next(n), ++i);
  return i;
}

bool strlist_contains(strlist *l, char *s) {
  strlist_node *n;

  for(n = strlist_first(l); n != NULL; n = strlist_next(n))
    if(!strcmp(strlist_val(n), s))
      return true;
  return false;
}

bool intlist_contains(intlist *l, int i) {
  intlist_node *n;

  for(n = intlist_first(l); n != NULL; n = intlist_next(n))
    if(intlist_val(n) == i)
      return true;
  return false;
}
