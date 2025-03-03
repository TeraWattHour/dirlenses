#include <dirent.h>
#include <ncurses.h>
#include <pthread.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#include "finfo.h"
#include "cache.h"

char *cwd() {
  char cwd[PATH_MAX + 1];
  getcwd(cwd, sizeof(cwd));
  return strdup(cwd);
}

int main() {
  int offset = 0;
  int focused = 0;

  cache_init();

  struct dir *dir = cache_add_dir(cwd());
  dir_retrieve_entries(dir);

  initscr();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  start_color();

  init_pair(1, COLOR_BLACK, COLOR_WHITE); // Highlighted
  init_pair(2, COLOR_WHITE, COLOR_BLACK); // Normal
  init_pair(3, COLOR_BLUE, COLOR_WHITE);  // Header

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

    for (int i = 0; i < h && i + offset < dir->count; i++) {
      const struct finfo *files = dir->files;
      const int idx = i + offset;

      attron(COLOR_PAIR(idx == focused ? 1 : 2));

      mvprintw(i + 1, 0, "%10s %s%s", files[idx].human_size, files[idx].name,
               files[idx].is_dir ? "/" : "");

      attroff(COLOR_PAIR(idx == focused ? 1 : 2));
    }

    refresh();

    switch (getch()) {
    case KEY_UP:
      focused = focused > 0 ? focused - 1 : dir->count - 1;
      break;
    case KEY_DOWN:
      focused = focused < dir->count - 1 ? focused + 1 : 0;
      break;
    case KEY_ENTER:
    case 10:
      if (dir->files[focused].is_dir) {
        chdir(dir->files[focused].name);

        dir = cache_add_dir(cwd());
        dir_retrieve_entries(dir);

        focused = 0;
        offset = 0;
      }
      break;

    case 's':
      dir_du(dir);
      break;

    case 'q':
      endwin();
      return 0;
    }
  }

  endwin();
  return 0;
}
