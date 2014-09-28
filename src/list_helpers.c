#include "list_helpers.h"
#include <string.h>

static bool s_is_child_of_n(const char *s, size_t len_s, const char *n, size_t len_n) {
  return !strncmp(s, n, len_n) && (len_n == len_s || (len_s > len_n && s[len_n] == '/'));
}

static bool test_s_is_child_of_n(const char *s, size_t len_s, const char *n, size_t len_n) {
  return s_is_child_of_n(s, len_s, n, len_n);
}

static bool test_s_is_parent_of_n(const char *s, size_t len_s, const char *n, size_t len_n)  {
  return s_is_child_of_n(n, len_n, s, len_s);
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
