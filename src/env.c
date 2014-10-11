#include "env.h"
#include "list_helpers.h"

#include <stdlib.h>
#include <string.h>

static void setup_environment_clean(char ***envp, strlist *keep_env, strlist *set_env) {
  size_t n, i;
  const char *key, *val;
  char *nkey, *pos;
  strlist_node *s;

  n = strlist_count(keep_env) + strlist_count(set_env);
  if( (*envp = (char **)malloc((n+1)*sizeof(char*))) == NULL )
    errExit("malloc");

  i = 0;

  for(s = strlist_first(set_env); s != NULL; s = strlist_next(s)) {
    val = strlist_val(s);
    if( (nkey = strdup(val)) == NULL )
      errExit("strdup");
    pos = index(nkey, '=');
    if(pos != NULL && pos != nkey) {
      *pos = '\0';
      strlist_remove(keep_env, nkey);
      *pos = '=';
      (*envp)[i++] = nkey;
    }
    else
      free(nkey);
  }

  for(s = strlist_first(keep_env); s != NULL; s = strlist_next(s)) {
    key = strlist_val(s);
    val = getenv(key);
    if( val != NULL) {
      if( ((*envp)[i] = (char*)malloc((strlen(key) + strlen(val) + 2)*sizeof(char))) == NULL )
        errExit("malloc");
      sprintf((*envp)[i++], "%s=%s", key, val);
    }
  }

  (*envp)[i] = NULL;
}

static void setup_environment_noclean(strlist *set_env) {
  strlist_node *s;
  const char *v;
  char *key, *pos;

  for(s = strlist_first(set_env); s != NULL; s = strlist_next(s)) {
    v = strlist_val(s);
    pos = index(v, '=');
    if(pos != NULL && pos != v) {
      if( (key = strndup(v, pos - v)) == NULL )
        errExit("strndup");
      setenv(key, pos + 1, 1);
      free(key);
    }
  }
}

void setup_environment(char ***envp, bool cleanenv, strlist *keep_env, strlist *set_env) {
  if(cleanenv)
    setup_environment_clean(envp, keep_env, set_env);
  else
    setup_environment_noclean(set_env);
}
