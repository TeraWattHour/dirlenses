#include "finfo.h"

int finfo_by_size(const void *a, const void *b) {
  struct finfo *lhs = (struct finfo *)a;
  struct finfo *rhs = (struct finfo *)b;

  return rhs->size - lhs->size;
}

int finfo_by_name(const void *a, const void *b) {
  return strcmp(((struct finfo *)a)->name, ((struct finfo *)b)->name);
}

int finfo_search_by_name(const void *key, const void *elem) {
  return strcmp((char *)key, ((struct finfo *)elem)->name);
}