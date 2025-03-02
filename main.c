#include <dirent.h>
#include <errno.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "finfo.h"

#define ERROR(fmt, ...)                              \
  {                                                  \
    endwin();                                        \
    fprintf(stderr, fmt __VA_OPT__(, ) __VA_ARGS__); \
    exit(1);                                         \
  }

char *human_readable_kb(int kb) {
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

static int offset = 0;
static int focused = 0;

static int n_files;
static bool dir_empty;
static struct finfo *files;
static pthread_mutex_t mutex;

void get_files() {
  DIR *dir = opendir(".");
  if (!dir)
    ERROR("couldn't open the current directory\n");

  int n = 1;
  int cap = 2;

  files = realloc(files, cap * sizeof(struct finfo));
  if (!files)
    ERROR("malloc failed\n");

  files[0] = (struct finfo){"..", 0, "", true};
  files[1] = (struct finfo){"(empty)", 0, "", false};

  struct stat *st = malloc(sizeof(struct stat));
  if (!st)
    ERROR("malloc failed\n");

  struct dirent *entry;
  while ((entry = readdir(dir))) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
      continue;

    if (n == cap) {
      cap *= 2;
      files = realloc(files, cap * sizeof(struct finfo));
      if (!files)
        ERROR("malloc failed\n");
    }

    files[n].name = strdup(entry->d_name);
    files[n].is_dir = entry->d_type == DT_DIR;
    if (entry->d_type == DT_DIR) {
      files[n].size = 0;
      files[n].human_size = "---";
    } else {
      if (stat(entry->d_name, st) != 0)
        ERROR("couldn't get file stats, errno: %d\n", errno);

      int kb = st->st_blocks / 2;
      files[n].size = kb;
      files[n].human_size = human_readable_kb(kb);
    }

    n++;
  }

  closedir(dir);
  free(st);

  // folder is actually empty, preserve the "(empty)" entry
  if (n == 1) {
    n_files = 2;
    dir_empty = true;
    return;
  };

  qsort(files + 2, n - 2, sizeof(struct finfo), finfo_by_name);

  n_files = n;
  return;
}

void *get_disk_usage_stats(void *arg) {
  pthread_mutex_lock(&mutex);
  FILE *f = popen(
      "find . -mindepth 1 -maxdepth 1 -print0 | xargs -0 du -sk 2>/dev/null",
      "r");
  if (!f)
    ERROR("couldn't get disk usage stats\n");

  char buffer[1024];
  while (fgets(buffer, 1024, f)) {
    int size = 0;
    char filename[1024] = {0};

    if (sscanf(buffer, "%d\t./%1023s", &size, filename) != 2) {
      continue;
    }

    struct finfo *info = bsearch(
        filename,
        files + 2,
        n_files - 2,
        sizeof(struct finfo),
        finfo_search_by_name);

    if (info) {
      info->human_size = human_readable_kb(size);
      int i = info - files;
      int idx = i + offset;

      attron(COLOR_PAIR(idx == focused ? 1 : 2));

      mvprintw(i + 1, 0, "%10s %s%s", files[idx].human_size, files[idx].name,
               files[idx].is_dir ? "/" : "");

      attroff(COLOR_PAIR(idx == focused ? 1 : 2));

      refresh();
    }
  }

  pclose(f);
  pthread_mutex_unlock(&mutex);
}

int main() {
  pthread_t threads[2];
  pthread_mutex_init(&mutex, NULL);

  initscr();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  start_color();

  init_pair(1, COLOR_BLACK, COLOR_WHITE); // Highlighted
  init_pair(2, COLOR_WHITE, COLOR_BLACK); // Normal
  init_pair(3, COLOR_BLUE, COLOR_WHITE);  // Header

  get_files();

  while (1) {
    clear();

    int w = getmaxx(stdscr);
    int h = getmaxy(stdscr) - 2;

    attron(COLOR_PAIR(3));
    mvprintw(0, 0, "%-*s", w, " Disk Usage Stats");
    mvprintw(h + 1, 0, "%-*s", w, " Use arrow keys to navigate, 'q' to quit");

    if (focused >= offset + h) {
      offset = focused - h + 1;
    } else if (focused < offset) {
      offset = focused;
    }

    for (int i = 0; i < h && i + offset < n_files; i++) {
      int idx = i + offset;

      attron(COLOR_PAIR(idx == focused ? 1 : 2));

      mvprintw(i + 1, 0, "%10s %s%s", files[idx].human_size, files[idx].name,
               files[idx].is_dir ? "/" : "");

      attroff(COLOR_PAIR(idx == focused ? 1 : 2));
    }

    refresh();

    switch (getch()) {
    case KEY_UP:
      focused = focused > 0 ? focused - 1 : n_files - 1;
      break;
    case KEY_DOWN:
      focused = focused < n_files - 1 ? focused + 1 : 0;
      break;
    case KEY_ENTER:
    case 10:
      if (files[focused].is_dir) {
        pthread_mutex_lock(&mutex);

        chdir(files[focused].name);
        get_files();

        focused = 0;
        offset = 0;

        pthread_mutex_unlock(&mutex);
      }
      break;

    case 's':
      pthread_create(&threads[0], NULL, get_disk_usage_stats, NULL);
      break;

    case 'q':
      endwin();
      return 0;
    }
  }

  endwin();
  pthread_mutex_destroy(&mutex);
  return 0;
}
