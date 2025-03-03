#ifndef CACHE_H
#define CACHE_H

#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "finfo.h"
#include "dir.h"
#include "error.h"

struct cache {
  struct dir **v;
  int count;
  int cap;
};

struct dir {
  pthread_mutex_t lock;
  char *name;
  struct finfo *files;
  int count;
};

void cache_init();
void cache_expand();
struct dir *cache_add_dir(const char *path) __attribute__((returns_nonnull));
struct dir *cache_find_dir(const char *path);
void cache_free();

#endif