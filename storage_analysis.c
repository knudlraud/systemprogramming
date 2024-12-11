/**************************************************************
 * storage_analysis.c
 * 2023006811 전하준
 * 2024 fall semester sysytem programming project.
 * visualizing the usage of the directory part of project.
 *************************************************************/

#define _DEFAULT_SOURCE
#include "storage_analysis.h"



typedef struct {
    char name[MAX_PATH];
    unsigned long size;
} DirEntry;

// Function prototypes
unsigned long get_dir_size(const char *path);
int is_absolute_directory(const char *path);
void get_directory_contents(const char *path, DirEntry *entries, int *count);

void convert_size(unsigned long size_bytes, char *result, size_t result_size) {
    if (size_bytes >= 1024 * 1024 * 1024) {
        snprintf(result, result_size, "%.2f GB", (double)size_bytes / (1024 * 1024 * 1024));
    } else if (size_bytes >= 1024 * 1024) {
        snprintf(result, result_size, "%.2f MB", (double)size_bytes / (1024 * 1024));
    } else if (size_bytes >= 1024) {
        snprintf(result, result_size, "%.2f KB", (double)size_bytes / 1024);
    } else {
        snprintf(result, result_size, "%lu B", size_bytes);
    }
}

void draw_bar(WINDOW *win, int y, int x, int width, double percentage) {
    int fill_width = (int)(percentage * (width-2));
    mvwprintw(win, y, x, "[");
    for (int i = 0; i < width-2; i++) {
        if (i < fill_width) {
            waddch(win, '#');
        } else {
            waddch(win, ' ');
        }
    }
    wprintw(win, "]\n  %.2f%%", percentage * 100);
}

int is_absolute_directory(const char *path) {
    if (path[0] != '/') {
        return 0;  // Not an absolute path
    }
    
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;  // Path does not exist
    }
    
    return S_ISDIR(statbuf.st_mode);
}

unsigned long get_file_size(const char *path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return st.st_size;
    }
    return 0;
}

unsigned long get_dir_size(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char full_path[MAX_PATH];
    unsigned long total_size = 0;

    dir = opendir(path);
    if (dir == NULL) {
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (entry->d_type == DT_DIR) {
            total_size += get_dir_size(full_path);
        } else {
            total_size += get_file_size(full_path);
        }
    }

    closedir(dir);
    return total_size;
}

void get_directory_contents(const char *path, DirEntry *entries, int *count) {
    DIR *dir;
    struct dirent *entry;
    char full_path[MAX_PATH];
    *count = 0;

    dir = opendir(path);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL && *count < MAX_ENTRIES) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        strncpy(entries[*count].name, entry->d_name, MAX_PATH - 1);
        entries[*count].name[MAX_PATH - 1] = '\0';

        if (entry->d_type == DT_DIR) {
            entries[*count].size = get_dir_size(full_path);
        } else {
            entries[*count].size = get_file_size(full_path);
        }

        (*count)++;
    }

    closedir(dir);
}

void display_info(WINDOW *win, const char* path, DirEntry *entries, int count, int page) {
    unsigned long total_size = get_dir_size(path);
    char size_str[20];

    int max_y, max_x;
    getmaxyx(win, max_y, max_x);
    int entries_per_page = (max_y - 7) / 3;  // Adjust based on available space

    wclear(win);
    mvwprintw(win, 1, 2, "Storage Usage for %s:", path);
    convert_size(total_size, size_str, sizeof(size_str));
    mvwprintw(win, 2, 2, "Total Size: %s", size_str);

    int start_index = page * entries_per_page;
    int end_index = (start_index + entries_per_page < count) ? start_index + entries_per_page : count;

    int start_y = 4;
    for (int i = start_index; i < end_index; i++) {
        double percentage = (double)entries[i].size / total_size;
        convert_size(entries[i].size, size_str, sizeof(size_str));
        mvwprintw(win, start_y + (i-start_index)*3, 2, "%s (%s):", entries[i].name, size_str);
        draw_bar(win, start_y + (i-start_index)*3 + 1, 2, max_x - 4, percentage);
    }

    mvwprintw(win, max_y - 2, 2, "Page %d/%d. Press 'p' for previous page, 'n' for next page, 'r' for new directory, 'q' to quit...", 
              page + 1, (count + entries_per_page - 1) / entries_per_page);
    box(win, 0, 0);
}

int storage_analysis_main() {
    setlocale(LC_ALL, "ko_KR.utf8");
    char path[MAX_PATH];
    DirEntry entries[MAX_ENTRIES];
    int count = 0;
    int page = 0;
    
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    scrollok(stdscr, FALSE);

    WINDOW *win = newwin(0, 0, 0, 0);
    box(win, 0, 0);
    scrollok(win, FALSE);

    int quit = 0;
    do {
        wclear(win);
        box(win, 0, 0);
        mvwprintw(win, 1, 2, "Enter absolute directory path: ");
        echo();
        wgetnstr(win, path, MAX_PATH);
        noecho();

        if (is_absolute_directory(path)) {
            get_directory_contents(path, entries, &count);
            page = 0;
            while (!quit) {
                display_info(win, path, entries, count, page);
                wrefresh(win);

                int ch = wgetch(win);
                if (ch == 'q') {
                    quit = 1;
                    break;
                }
                if (ch == 'r') break;
                
                int max_y, max_x;
                getmaxyx(win, max_y, max_x);
                int entries_per_page = (max_y - 7) / 3;
                
                if (ch == 'n' && (page + 1) * entries_per_page < count) page++;
                if (ch == 'p' && page > 0) page--;

                napms(100);  // Wait for 0.1 second
            }
        } else {
            wclear(win);
            mvwprintw(win, 1, 2, "Error: Invalid absolute directory path %s", path);
            mvwprintw(win, 3, 2, "Press any key to try again, 'q' to quit");
            box(win, 0, 0);
            wrefresh(win);
            int ch = wgetch(win);
            if (ch == 'q') quit = 1;
        }
    } while (!quit);

    // Cleanup ncurses
    endwin();

    return 0;
}
