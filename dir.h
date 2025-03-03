#ifndef DIR_H
#define DIR_H
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>

#include "cache.h"

int dir_retrieve_entries(struct dir *dir);
struct finfo *dir_get_entries(struct dir *dir);
void dir_du(struct dir *dir);

#endif