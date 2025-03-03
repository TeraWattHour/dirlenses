#include "finfo.h"

#include <string.h>

int finfo_by_size(const void *a, const void *b) {
  const struct finfo *lhs = a;
  const struct finfo *rhs = b;

  return rhs->size - lhs->size;
}

// ".." is always the first entry, then comes ".", then the rest (which is optional) sorted alphabetically (strcmp)
int by_name(const char *a, const char *b) {
  if (strcmp(a, "..") == 0) return -1;
  if (strcmp(b, "..") == 0) return 1;

  if (strcmp(a, ".") == 0) return -1;
  if (strcmp(b, ".") == 0) return 1;

  return strcmp(a, b);
}


int finfo_by_name(const void *a, const void *b) {
  const struct finfo *lhs = a;
  const struct finfo *rhs = b;
  return by_name(lhs->name, rhs->name);
}


int finfo_search_by_name(const void *key, const void *elem) {
  const struct finfo *rhs = elem;
  return by_name(key, rhs->name);
}