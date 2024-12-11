#ifndef SEARCH_H
#define SEARCH_H

#define _XOPEN_SOURCE 700 // Enable POSIX extensions like strptime
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <ncurses.h>

#define PAGE_SIZE 10
#define MAX_RESULTS 100000
#define PATH_MAX 4096


void search_files();

#endif
