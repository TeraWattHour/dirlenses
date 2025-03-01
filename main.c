#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define ERROR(fmt, ...)                              \
  {                                                  \
    endwin();                                        \
    fprintf(stderr, fmt __VA_OPT__(, ) __VA_ARGS__); \
    exit(1);                                         \
  }

struct finfo {
  char *name;
  int size;
  char *human_size;
  bool is_dir;
};

int sorting(const void *a, const void *b) {
  struct finfo *lhs = (struct finfo *)a;
  struct finfo *rhs = (struct finfo *)b;

  return rhs->size - lhs->size;
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

int get_disk_usage_stats(struct finfo **files) {
  FILE *f = popen("ls | wc -l 2>/dev/null", "r");
  if (!f) ERROR("couldn't count files in directory\n");

  char buffer[1024];
  if (!fgets(buffer, sizeof(buffer), f)) ERROR("couldn't read from pipe\n");
  int n = atoi(buffer) + 2;
  pclose(f);

  *files = realloc(*files, n * sizeof(struct finfo));
  if (!files) ERROR("malloc failed\n");

  (*files)[0] = (struct finfo){"..", 0, "", true};
  (*files)[1] = (struct finfo){".", 0, "", true};

  if (n == 2) {
    *files = realloc(*files, 3 * sizeof(struct finfo));
    (*files)[2] = (struct finfo){"(empty)", 0, "", false};

    return 3;
  }

  f = popen("du -sk -- * 2>/dev/null", "r");
  if (!f) ERROR("couldn't get disk usage stats\n");

  for (int i = 2; i < n; i++) {
    if (fscanf(f, "%d\t%255[^\n]", &(*files)[i].size, buffer) != 2)
      ERROR("couldn't read file sizes\n");

    (*files)[i].name = strdup(buffer);
    (*files)[i].human_size = human_readable_kb((*files)[i].size);
  }

  pclose(f);

  f = popen("ls -l 2>/dev/null", "r");
  if (!f) ERROR("couldn't get file types\n");

  for (int i = -1; i < n - 2; i++) {
    if (!fgets(buffer, sizeof(buffer), f)) ERROR("couldn't read from pipe\n");
    if (i == -1) continue;

    (*files)[i + 2].is_dir = buffer[0] == 'd';
  }

  pclose(f);

  qsort(*files + 2, n - 2, sizeof(struct finfo), sorting);

  return n;
}

int main() {
  initscr();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  start_color();

  init_pair(1, COLOR_BLACK, COLOR_WHITE);  // Highlighted
  init_pair(2, COLOR_WHITE, COLOR_BLACK);  // Normal
  init_pair(3, COLOR_BLUE, COLOR_WHITE);   // Header

  struct finfo *files = NULL;
  int n = get_disk_usage_stats(&files);

  int focused = 0;
  int offset = 0;

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

    for (int i = 0; i < h && i + offset < n; i++) {
      int idx = i + offset;
      attron(COLOR_PAIR(idx == focused ? 1 : 2));
      mvprintw(i + 1, 0, "%10s %s%s", files[idx].human_size, files[idx].name,
               files[idx].is_dir ? "/" : "");
      attroff(COLOR_PAIR(idx == focused ? 1 : 2));
    }

    refresh();

    switch (getch()) {
      case KEY_UP:
        focused = focused > 0 ? focused - 1 : n - 1;
        break;
      case KEY_DOWN:
        focused = focused < n - 1 ? focused + 1 : 0;
        break;
      case KEY_ENTER:
      case 10:
        if (files[focused].is_dir) {
          chdir(files[focused].name);
          n = get_disk_usage_stats(&files);
          focused = 0;
          offset = 0;
        }
        break;
      case 'q':
        endwin();
        return 0;
    }
  }

  endwin();
  return 0;
}
