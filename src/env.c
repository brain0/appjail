#include "env.h"
#include "list_helpers.h"

#include <stdlib.h>
#include <string.h>

void setup_environment(char ***envp, strlist *keep_env) {
  size_t n, i;
  const char *key, *val;
  strlist_node *s;

  n = strlist_count(keep_env);
  *envp = (char **)malloc((n+1)*sizeof(char*));
  i = 0;
  for(s = strlist_first(keep_env); s != NULL; s = strlist_next(s)) {
    key = strlist_val(s);
    val = getenv(key);
    if( val != NULL) {
      (*envp)[ i ] = (char*)malloc((strlen(key) + strlen(val) + 2)*sizeof(char));
      sprintf((*envp)[i], "%s=%s", key, val);
      i++;
    }
  }
  (*envp)[i] = NULL;
}
