#ifndef ERROR_H
#define ERROR_H
#include <ncurses.h>

#define ERROR(fmt, ...)                              \
  ({                                                 \
    endwin();                                        \
    fprintf(stderr, fmt __VA_OPT__(, ) __VA_ARGS__); \
    exit(1);                                         \
  })

#endif