#pragma once

struct strlist;
struct strlist_node;

typedef struct strlist strlist;
typedef struct strlist_node strlist_node;

strlist *strlist_new();
void strlist_free(strlist *l);

void strlist_append(strlist *l, char *s);
void strlist_append_copy(strlist *l, const char *s);
void strlist_append_copy_unique(strlist *l, const char *s);
strlist_node *strlist_first(strlist *l);
strlist_node *strlist_next(strlist_node *n);
const char *strlist_val(strlist_node *n);

struct intlist;
struct intlist_node;

typedef struct intlist intlist;
typedef struct intlist_node intlist_node;

intlist *intlist_new();
void intlist_free(intlist *l);

void intlist_append(intlist *l, int i);
intlist_node *intlist_first(intlist *l);
intlist_node *intlist_next(intlist_node *n);
int intlist_val(intlist_node *n);
