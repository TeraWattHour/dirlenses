#ifndef DIR_H
#define DIR_H

#include "cache.h"

int dir_retrieve_entries(struct dir *dir);
struct finfo *dir_get_entries(struct dir *dir);

struct dir_with_render {
  struct dir *dir;
  void (*render)();
};

void *wrapped_dir_du(void *arg);
void dir_du(struct dir *dir, void (*render)());

#endif