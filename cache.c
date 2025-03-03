#include "cache.h"

#include "error.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

static struct cache c;

unsigned long hash(const char *str);

void cache_init() {
  c.count = 0;
  c.cap = 16;

  c.v = malloc(sizeof(struct dir *) * c.cap);
  if (!c.v) ERROR("malloc failed with error code %d\n", errno);

  memset(c.v, 0, sizeof(struct dir *) * c.cap);
}

struct dir *cache_add_dir(const char *path) {
  struct dir *existing = cache_find_dir(path);
  if (existing) return existing;

  if (c.count >= c.cap * 0.7) {
    cache_expand();
  }

  unsigned long destination = hash(path) % c.cap;
  while (c.v[destination]) {
    destination = (destination + 1) % c.cap;
  }

  c.v[destination] = malloc(sizeof(struct dir));
  if (!c.v[destination]) ERROR("malloc failed with error code %d\n", errno);

  c.v[destination]->name = strdup(path);
  pthread_mutex_init(&c.v[destination]->lock, NULL);

  c.count++;

  return c.v[destination];
}

struct dir *cache_find_dir(const char *path) {
  const unsigned long destination = hash(path) % c.cap;

  for (int i = 0; i < c.cap; i++) {
    const unsigned long idx = (destination + i) % c.cap;
    if (c.v[idx] && strcmp(c.v[idx]->name, path) == 0) return c.v[idx];
  }

  return NULL;
}

void cache_expand() {
  c.cap *= 2;

  struct dir **resized = malloc(sizeof(struct dir *) * c.cap);
  if (!resized) ERROR("realloc failed with error code %d\n", errno);
  memset(resized, 0, sizeof(struct dir *) * c.cap);

  for (int i = 0; i < c.count; i++) {
    unsigned long place = hash(c.v[i]->name) % c.cap;
    while (c.v[place]) { place = (place + 1) % c.cap; }
    resized[place] = malloc(sizeof(struct dir));
    memcpy(resized[place], c.v[i], sizeof(struct dir));
  }

  free(c.v);
  c.v = resized;
}

void cache_free() {
  for (int i = 0; i < c.count; i++) {
    free(c.v[i]->name);
    free(c.v[i]->files);
    pthread_mutex_destroy(&c.v[i]->lock);
    free(c.v[i]);
  }
  free(c.v);
}

unsigned long hash(const char *str) {
  const unsigned char *p = (const unsigned char*)str;
  unsigned long hash = 5381;
  int c;

  while ((c = *p++))
    hash = (hash << 5) + hash + c;

  return hash;
}
