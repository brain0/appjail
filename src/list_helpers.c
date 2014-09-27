#include "list_helpers.h"
#include <string.h>

bool has_path(strlist *l, const char *needle, has_path_mode_t mode) {
  strlist_node *i;
  size_t len, len_needle;

  len_needle = strlen(needle);
  /* Deal with trailing slashes */
  if(needle[len_needle-1] == '/')
    len_needle--;
  for(i = strlist_first(l); i != NULL; i = strlist_next(i)) {
    len = strlen(strlist_val(i));
    switch(mode) {
      case HAS_CHILD_OF_NEEDLE:
        if(!strncmp(strlist_val(i), needle, len_needle) && (len_needle == len || (len > len_needle && strlist_val(i)[len_needle] == '/')))
          return true;
        break;
      case HAS_PARENT_OF_NEEDLE:
        if(!strncmp(strlist_val(i), needle, len) && (len_needle == len || (len_needle > len && needle[len] == '/')))
          return true;
        break;
      case HAS_EXACT_PATH:
        if(!strcmp(strlist_val(i), needle))
          return true;
        break;
    }
  }

  return false;
}
