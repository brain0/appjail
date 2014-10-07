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

void strlist_append_copy_unique(strlist *l, const char *s) {
  strlist_node *n;

  for(n = strlist_first(l); n != NULL; n = strlist_next(n))
    if(!strcmp(strlist_val(n), s))
      return;
  strlist_append_copy(l, s);
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

struct intlist_node {
  int val;
  intlist_node *next;
};

struct intlist {
  intlist_node *first;
  intlist_node *last;
};

intlist *intlist_new() {
  intlist *ret;

  if( (ret = malloc(sizeof(intlist))) == NULL )
    errExit("malloc");
  ret->first = NULL;
  ret->last = NULL;

  return ret;
}

void intlist_free(intlist *l) {
  intlist_node *cur, *next;

  cur = l->first;
  while( cur != NULL ) {
    next = cur->next;
    free( cur );
    cur = next;
  }
  free( l );
}

void intlist_append(intlist *l, int i) {
  intlist_node *n;

  if( (n = malloc(sizeof(intlist_node))) == NULL )
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
  n->val = i;
}

intlist_node *intlist_first(intlist *l) {
  return l->first;
}

intlist_node *intlist_next(intlist_node *n) {
  return n->next;
}

int intlist_val(intlist_node *n) {
  return n->val;
}
