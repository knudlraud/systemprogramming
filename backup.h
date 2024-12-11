#ifndef BACKUP_H
#define BACKUP_H


#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include <limits.h>
#include <libgen.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <pthread.h> 
#include <curses.h>

int backup_main();

#endif
