#include "list.h"
#include "common.h"
#include <stdlib.h>
#include <string.h>

struct strlist_node {
  char *val;
  strlist_node *next;
};

struct strlist {
  strlist_node *first;
  strlist_node *last;
};

strlist *strlist_new() {
  strlist *ret;

  if( (ret = malloc(sizeof(strlist))) == NULL )
    errExit("malloc");
  ret->first = NULL;
  ret->last = NULL;

  return ret;
}

void strlist_free(strlist *l) {
  strlist_node *cur, *next;

  cur = l->first;
  while( cur != NULL ) {
    next = cur->next;
    free( cur->val );
    free( cur );
    cur = next;
  }
  free( l );
}

static strlist_node *add_node(strlist *l) {
  strlist_node *n;

  if( (n = malloc(sizeof(strlist_node))) == NULL )
    errExit("malloc");
  if( l->last == NULL ) {
    l->first = n;
    l->last = n;
  }
  else {
    l->last->next = n;
    l->last = n;
  }
  n->next = NULL;

  return n;
}

void strlist_append(strlist *l, char *s) {
  add_node(l)->val = s;
}

void strlist_append_copy(strlist *l, const char *s) {
  add_node(l)->val = strdup(s);
}

strlist_node *strlist_first(strlist *l) {
  return l->first;
}

strlist_node *strlist_next(strlist_node *n) {
  return n->next;
}

const char *strlist_val(strlist_node *n) {
  return n->val;
}
