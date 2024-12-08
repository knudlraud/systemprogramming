#ifndef MONIBACKUP_H
#define MONIBACKUP_H

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
