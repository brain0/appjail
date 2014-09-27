#pragma once

struct strlist;
struct strlist_node;

typedef struct strlist strlist;
typedef struct strlist_node strlist_node;

strlist *strlist_new();
void strlist_free(strlist *l);

void strlist_append(strlist *l, char *s);
void strlist_append_copy(strlist *l, const char *s);
strlist_node *strlist_first(strlist *l);
strlist_node *strlist_next(strlist_node *n);
const char *strlist_val(strlist_node *n);
