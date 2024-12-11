

/**************************************************************
 * moni.c
 * 2023014973 노성민, 2023004915 장시훈
 * 2024 fall semester sysytem programming project.
 * Backing up and monitoring with visualize part of project.
 *************************************************************/
#include "moni.h"



void rotate_log_file(const char* log_filepath) {
    struct stat st;
    if (stat(log_filepath, &st) == -1) {
        perror("Failed to stat log file");
        return;
    }

    if (st.st_size > 1024 * 1024) {
        char old_log[PATH_MAX];
        snprintf(old_log, sizeof(old_log), "%s.old", log_filepath);
        if (rename(log_filepath, old_log) == -1) {
            perror("Failed to rotate log file");
        }
    }
}

void delete_old_logs(const char* log_directory) {
    DIR* dir = opendir(log_directory);
    if (dir == NULL) {
        perror("Failed to open log directory");
        return;
    }

    struct dirent* entry;
    time_t now = time(NULL);
    const int DAYS_TO_KEEP = 7;
    const time_t SECONDS_IN_A_DAY = 86400;

    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".old") != NULL) {
            char filepath[PATH_MAX];
            snprintf(filepath, sizeof(filepath), "%s/%s", log_directory, entry->d_name);

            struct stat st;
            if (stat(filepath, &st) == -1) {
                perror("Failed to stat old log file");
                continue;
            }

            if (difftime(now, st.st_mtime) > (DAYS_TO_KEEP * SECONDS_IN_A_DAY)) {
                if (remove(filepath) == -1) {
                    perror("Failed to remove old log file");
                }
                else {
                    printf("Deleted old log file: %s\n", filepath);
                }
            }
        }
    }

    closedir(dir);
}

void log_event(const char* log_filepath, const char* event_type, const char* file_path) {
    FILE* log_file = fopen(log_filepath, "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        return;
    }
    time_t now = time(NULL);
    char* timestamp = ctime(&now);
    timestamp[strcspn(timestamp, "\n")] = 0;
    fprintf(log_file, "%s - %s: %s\n", timestamp, event_type, file_path);
    fclose(log_file);
}

pthread_t threads[MAX_DIRS];
char directories[MAX_DIRS][PATH_MAX]; // 정적 배열로 변경
int dir_count = 0;

void* monitor_directory(void* arg) {
    const char* dir_path = (const char*)arg;
    const char* log_filepath = "monitor.log";

    int fd = inotify_init();
    if (fd < 0) {
        perror("inotify_init");
        return NULL;
    }

    int wd = inotify_add_watch(fd, dir_path, IN_MODIFY | IN_CREATE | IN_DELETE);
    if (wd == -1) {
        perror("inotify_add_watch");
        close(fd);
        return NULL;
    }

    char buffer[BUF_LEN];
    while (1) {
        int length = read(fd, buffer, BUF_LEN);
        if (length < 0) {
            perror("read");
            break;
        }
        if (length == 0) {
            continue;
        }

        int i = 0;
        while (i < length) {
            struct inotify_event* event = (struct inotify_event*)&buffer[i];
            if (event->len) {
                if (event->mask & IN_CREATE) {
                    log_event(log_filepath, "File created", event->name);
                }
                else if (event->mask & IN_DELETE) {
                    log_event(log_filepath, "File deleted", event->name);
                }
                else if (event->mask & IN_MODIFY) {
                    log_event(log_filepath, "File modified", event->name);
                }
                rotate_log_file(log_filepath);
                delete_old_logs(".");
            }
            i += EVENT_SIZE + event->len;
        }
    }

    inotify_rm_watch(fd, wd);
    close(fd);
    return NULL;
}

void display_menu(WINDOW* win) {
    int height, width;
    getmaxyx(win, height, width);

    int text_y = height / 4;
    int text_x = (width - strlen("Advanced File Explorer")) / 2;

    mvwprintw(win, text_y, text_x, "Advanced File Explorer");

    const char* items[] = { "1. Exit", "2. Start Monitoring", "3. Show Monitoring Status", "4. Stop Monitoring", "5. View Log File" };
    for (int i = 0; i < 5; i++) {
        mvwprintw(win, text_y + (height / 5) + (2 * i), text_x, "%s", items[i]);
    }

    wrefresh(win);
}

void display_monitoring_status(WINDOW* win) {
    wclear(win);
    box(win, 0, 0);
    mvwprintw(win, 1, 1, "Currently Monitoring Directories:");
    for (int i = 0; i < dir_count; i++) {
        mvwprintw(win, 2 + i, 1, "%d: %s", i + 1, directories[i]);
    }
    mvwprintw(win, 2 + dir_count + 1, 1, "Press any key to return to menu.");
    wrefresh(win);
    wgetch(win);

    wclear(win);
    box(win, 0, 0);
    display_menu(win);
}

void stop_monitoring(WINDOW* win) {
    for (int i = 0; i < dir_count; i++) {
        pthread_cancel(threads[i]);
        pthread_join(threads[i], NULL);
    }
    dir_count = 0;
    
    wclear(win);
    box(win, 0, 0);
    mvwprintw(win, 1, 1, "Monitoring stopped.");
    wrefresh(win);
    napms(1000); 

    wclear(win);
    box(win, 0, 0);
    display_menu(win); 
    
}

void view_log_file(WINDOW* win) {
    wclear(win);
    box(win, 0, 0);
    mvwprintw(win, 1, 1, "Log File (Press 'q' to exit):\n");
    wrefresh(win);

    FILE* log_file = fopen("monitor.log", "r");
    if (log_file == NULL) {
        mvwprintw(win, 2, 1, "Failed to open log file.\n");
        wrefresh(win);
        int ch = wgetch(win);
        if (ch == 'q')
        {
            wclear(win);
            box(win, 0, 0);
            return;
        }
    }

    char line[256];
    while (fgets(line, sizeof(line), log_file)) {
        wprintw(win, "%s", line);
        wrefresh(win);
        napms(100);
    }

    fclose(log_file);

    int ch;
    while (1) {
        ch = wgetch(win);
        if (ch == 'q') {
            break;
        }
    }

    wclear(win);
    box(win, 0, 0);
    display_menu(win);
}

void input_directories(WINDOW* win) {
    int height, width;
    getmaxyx(win, height, width);

    mvwprintw(win, height - 2, (width - strlen("Enter 'done' when finished")) / 2, "Enter 'done' when finished");
    wrefresh(win);

    mvwprintw(win, 1, 1, "Enter directories to monitor:");
    wrefresh(win);

    for (int i = 0; i < MAX_DIRS; i++) {
        char input[PATH_MAX] = { 0 };
        int pos = 0;
        mvwprintw(win, 3 + i, 1, "Directory %d: ", i + 1);
        wrefresh(win);

        while (1) {
            int ch = wgetch(win);

            if (ch == '\n') {
                input[pos] = '\0';
                break;
            }
            else if (ch == KEY_BACKSPACE || ch == 127) {
                if (pos > 0) {
                    pos--;
                    input[pos] = '\0';
                }
            }
            else if (pos < PATH_MAX - 1 && (ch >= ' ' && ch <= '~')) {
                input[pos++] = ch;
                input[pos] = '\0';
            }

            wclear(win);
            mvwprintw(win, height - 2, (width - strlen("Enter 'done' when finished")) / 2, "Enter 'done' when finished");
            mvwprintw(win, 1, 1, "Enter directories to monitor:");

            for (int j = 0; j < i; j++) {
                mvwprintw(win, 3 + j, 1, "Directory %d: %s", j + 1, directories[j]);
            }
            mvwprintw(win, 3 + i, 1, "Directory %d: %s", i + 1, input);
            wrefresh(win);
        }

        if (strcmp(input, "done") == 0) {
            wclear(win);
            box(win, 0, 0);
            mvwprintw(win, height / 2, (width - strlen("Returning to menu...")) / 2, "Returning to menu...");
            wrefresh(win);
            napms(1000);

            
            wclear(win);
            box(win, 0, 0);
            display_menu(win);
            
            break;
        }

        if (input[0] == '~') {
            const char* home = getenv("HOME");
            char expanded[PATH_MAX];
            snprintf(expanded, sizeof(expanded), "%s%s", home, input + 1);
            strncpy(input, expanded, PATH_MAX);
        }

        struct stat path_stat;
        if (stat(input, &path_stat) != 0 || !S_ISDIR(path_stat.st_mode)) {
            mvwprintw(win, 3 + i + 1, 1, "Invalid directory. Try again.");
            wrefresh(win);
            i--;
            continue;
        }

        // 정적 배열에 입력된 경로를 복사
        strncpy(directories[i], input, PATH_MAX);
        dir_count++;
    }
}

int moni_main() {
    setlocale(LC_ALL, "en_US.UTF-8");
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    WINDOW* win = newwin(0, 0, 0, 0);
    box(win, 0, 0);

    do {
        display_menu(win);
        int ch = wgetch(win);
        if (ch == '1') {
            break;
        }
        else if (ch == '2') {
            wclear(win);
            input_directories(win);

            for (int i = 0; i < dir_count; i++) {
                if (pthread_create(&threads[i], NULL, monitor_directory, directories[i]) != 0) {
                    perror("pthread_create");
                    exit(EXIT_FAILURE);
                }
            }
        }
        else if (ch == '3') {
            wclear(win);
            display_monitoring_status(win);
        }
        else if (ch == '4') {
            stop_monitoring(win);
        }
        else if (ch == '5') {
            view_log_file(win);
        }

    } while (1);

    delwin(win);
    endwin();

    return 0;
}

