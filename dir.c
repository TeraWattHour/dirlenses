#include "dir.h"


char *human_readable_kb(long long kb) {
  char *buffer = malloc(16);
  if (kb < 1024) {
    sprintf(buffer, "%dK", kb);
  } else if (kb < 1024 * 1024) {
    sprintf(buffer, "%dM", kb / 1024);
  } else {
    sprintf(buffer, "%dG", kb / (1024 * 1024));
  }

  return buffer;
}

int dir_retrieve_entries(struct dir *dir) {
  if (dir->count > 0) return -1;

  struct dirent **entries;
  const int count = scandir(".", &entries, 0, alphasort);
  if (count < 0) return errno;

  dir->count = count;
  struct finfo *tmp = realloc(dir->files, sizeof(struct finfo) * dir->count);
  if (tmp == NULL) return errno;
  dir->files = tmp;

  struct stat *st = malloc(sizeof(struct stat));

  int j = 0;
  for (int i = 0; i < count; i++) {
    struct dirent *entry = entries[i];
    dir->files[j].name = strdup(entry->d_name);
    dir->files[j].is_dir = entry->d_type == DT_DIR;
    if (!dir->files[j].is_dir) {

      // TODO: handle errors
      stat(entry->d_name, st) != 0;

      const long long kb = st->st_blocks / 2;
      dir->files[j].size = kb;
      dir->files[j].human_size = human_readable_kb(kb);
    } else {
      dir->files[j].size = -1;
      dir->files[j].human_size = "---";
    }

    j++;
  }

  free(st);
  free(entries);

  qsort(dir->files, dir->count, sizeof(struct finfo), finfo_by_name);

  return 0;
}

void *dir_du_(void *arg) {
  struct dir *dir = arg;

  pthread_mutex_lock(&dir->lock);

  FILE *f = popen(
    "find . -mindepth 1 -maxdepth 1 -print0 | xargs -0 du -sk 2>/dev/null",
    "r"
  );
  // if (!f) ERROR("couldn't get disk usage stats\n");

  char buffer[1024];
  while (fgets(buffer, 1024, f)) {
    int size = 0;
    char filename[1024] = {0};

    sscanf(buffer, "%d\t./%1023s", &size, filename);

    struct finfo *info = bsearch(
      filename,
      dir->files,
      dir->count,
      sizeof(struct finfo),
      finfo_search_by_name
    );

    if (info) {
      info->human_size = human_readable_kb(size);
      // int i = info - files;
      // int idx = i + offset;
      //
      // attron(COLOR_PAIR(idx == focused ? 1 : 2));
      //
      // mvprintw(i + 1, 0, "%10s %s%s", files[idx].human_size, files[idx].name,
      //          files[idx].is_dir ? "/" : "");
      //
      // attroff(COLOR_PAIR(idx == focused ? 1 : 2));
      //
      // refresh();
    }
  }

  pclose(f);
  pthread_mutex_unlock(&dir->lock);
  return NULL;
}

void dir_du(struct dir *dir) {
  static pthread_t thread;
  pthread_create(&thread, NULL, dir_du_, dir);
}

