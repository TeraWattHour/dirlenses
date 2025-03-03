#ifndef FINFO_H
#define FINFO_H

#include <stdbool.h>
#include <string.h>

struct finfo {
  char *name;
  long long size;
  char *human_size;
  bool is_dir;
};

int finfo_by_size(const void *a, const void *b);
int finfo_by_name(const void *a, const void *b);
int finfo_search_by_name(const void *key, const void *elem);

#endif