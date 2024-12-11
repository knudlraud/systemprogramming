/**************************************************************
 * search_files.c
 * 2019115817 장성욱, 2020110621 이수민
 * 2024 fall semester sysytem programming project.
 * Implementation of search and visualize part of project.
 *************************************************************/

#include "search_files.h"

// 필터링에 쓰일 파일 정보 구조체
typedef struct
{
    char path[PATH_MAX];
    size_t size;
    time_t mtime;
} FileInfo;

FileInfo results[MAX_RESULTS];
int result_count = 0;

// 시간 측정용
double calculate_elapsed_time(clock_t start, clock_t end)
{
    return (double)(end - start) / CLOCKS_PER_SEC;
}

// 파일 메타데이터 수집
void collect_file_info(const char *path, const struct stat *file_stat)
{
    if (result_count >= MAX_RESULTS)
        return;

    FileInfo info;
    strncpy(info.path, path, PATH_MAX - 1);
    info.size = file_stat->st_size;
    info.mtime = file_stat->st_mtime;

    results[result_count++] = info;
}

// 디렉토리 재귀탐색
void search_directory(const char *path, size_t min_size, size_t max_size, time_t min_mtime, time_t max_mtime)
{
    struct dirent *entry;
    struct stat file_stat;
    DIR *dir = opendir(path);

    if (!dir)
    {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        // "." 과 ".."이면 스킵
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
#ifdef DEBUG
            printf("curruent: %s, continue.\n", entry->d_name);
#endif
            continue;
        }

#ifdef DEBUG
        printf("current: %s, ", entry->d_name);
#endif

        // 파일 절대 경로 생성
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
#ifdef DEBUG
        printf("full path: %s\n", full_path);
#endif

        // 절대경로 기반 파일 메타데이터 획득
        if (stat(full_path, &file_stat) == -1)
        {
            perror("stat");
            continue;
        }

#ifdef DEBUG
        printf("stat susccess.\n");
#endif

        // 디렉토리면 들어가서 탐색
        if (S_ISDIR(file_stat.st_mode))
        {
#ifdef DEBUG
            printf("found directory. Going in.\n");
#endif
            search_directory(full_path, min_size, max_size, min_mtime, max_mtime);
        }
        else if (S_ISREG(file_stat.st_mode))
        {
            // 모든 기준을 적용했을때
            if (file_stat.st_size >= min_size && file_stat.st_size <= max_size &&
                file_stat.st_mtime >= min_mtime && file_stat.st_mtime <= max_mtime)
            {
                collect_file_info(full_path, &file_stat);
            }
        }
    }

    closedir(dir);
}

// 크기 기준
int compare_by_size(const void *a, const void *b)
{
    return ((FileInfo *)a)->size - ((FileInfo *)b)->size;
}

// 수정 시간 기준
int compare_by_mtime(const void *a, const void *b)
{
    return ((FileInfo *)a)->mtime - ((FileInfo *)b)->mtime;
}

// 페이지로 변환
void paginate_results(WINDOW *win)
{
    int page = 0;
    int total_pages = (result_count + PAGE_SIZE - 1) / PAGE_SIZE;
    char *mtime_str = NULL;
    int start, end;
    int rows, cols;

    getmaxyx(win, rows, cols);

    int min_path_width = 20;
    int min_size_width = 12;
    int min_mtime_width = 15;

    float path_ratio = 0.5f;
    float size_ratio = 0.3f;
    float mtime_ratio = 0.2f;

    // 상황에 따라 열 비율 조정
    if (cols < 80)
    {
        path_ratio = 0.6f;
        size_ratio = 0.2f;
        mtime_ratio = 0.2f;
    }
    else if (cols < 100)
    {
        path_ratio = 0.55f;
        size_ratio = 0.25f;
        mtime_ratio = 0.2f;
    }

    int path_width = (int)(cols * path_ratio);
    int size_width = (int)(cols * size_ratio);
    int mtime_width = (int)(cols * mtime_ratio);

    if (path_width < min_path_width)
        path_width = min_path_width;
    if (size_width < min_size_width)
        size_width = min_size_width;
    if (mtime_width < min_mtime_width)
        mtime_width = min_mtime_width;

    while (1)
    {
        start = page * PAGE_SIZE;
        end = start + PAGE_SIZE;
        if (end > result_count)
            end = result_count;

        wclear(win);

        // 동적으로 테두리의 크기 계산
        for (int i = 0; i < cols; i++)
        {
            mvwaddch(win, 0, i, ACS_HLINE);
            mvwaddch(win, rows - 1, i, ACS_HLINE);
        }
        for (int i = 0; i < rows; i++)
        {
            mvwaddch(win, i, 0, ACS_VLINE);
            mvwaddch(win, i, cols - 1, ACS_VLINE);
        }

        mvwaddch(win, 0, 0, ACS_ULCORNER);
        mvwaddch(win, 0, cols - 1, ACS_URCORNER);
        mvwaddch(win, rows - 1, 0, ACS_LLCORNER);
        mvwaddch(win, rows - 1, cols - 1, ACS_LRCORNER);

        // 파일 정보 헤더 출력
        mvwprintw(win, 1, 2, "Path");
        mvwprintw(win, 1, path_width + 2, "Size");
        mvwprintw(win, 1, path_width + size_width + 2, "Modified");

        for (int i = 0; i < cols; i++)
            mvwaddch(win, 2, i, ACS_HLINE);
        for (int i = start; i < end; ++i)
        {
            mtime_str = ctime(&results[i].mtime);
            mtime_str[strcspn(mtime_str, "\n")] = '\0';

            char path_display[path_width - 4];
            snprintf(path_display, sizeof(path_display), "%s", results[i].path);

            // '...'을 이용하여 생략
            if (strlen(path_display) > path_width - 7)
            {
                path_display[path_width - 7] = '.';
                path_display[path_width - 6] = '.';
                path_display[path_width - 5] = '.';
                path_display[path_width - 4] = '\0';
            }

            char size_display[size_width - 4];
            snprintf(size_display, sizeof(size_display), "%lu bytes", results[i].size);
            if (strlen(size_display) > size_width - 7)
            {
                size_display[size_width - 7] = '.';
                size_display[size_width - 6] = '.';
                size_display[size_width - 5] = '.';
                size_display[size_width - 4] = '\0';
            }

            char mtime_display[mtime_width - 4];
            snprintf(mtime_display, sizeof(mtime_display), "%s", mtime_str);
            if (strlen(mtime_display) > mtime_width - 7)
            {
                mtime_display[mtime_width - 7] = '.';
                mtime_display[mtime_width - 6] = '.';
                mtime_display[mtime_width - 5] = '.';
                mtime_display[mtime_width - 4] = '\0';
            }

            mvwprintw(win, i - start + 3, 2, "%-*s", path_width - 2, path_display);
            mvwprintw(win, i - start + 3, path_width + 2, "%-*s", size_width - 2, size_display);
            mvwprintw(win, i - start + 3, path_width + size_width + 2, "%-*s", mtime_width - 2, mtime_display);
        }

        char page_info[128];
        snprintf(page_info, sizeof(page_info), "--- Page %d of %d ---", page + 1, total_pages);
        int page_info_len = strlen(page_info);
        mvwprintw(win, rows - 2, (cols - page_info_len) / 2, "%s", page_info);

        mvwprintw(win, rows - 3, 2, "Press 'p' for previous page, 'n' for next page, 'q' to quit...");

        wrefresh(win);

        char next = wgetch(win);
        if (next == 'q')
            break;

        if (next == 'n' && page < total_pages - 1)
            page++;

        if (next == 'p' && page > 0) 
            page--;
    }
}

// 파일로 저장
void save_results(const char *filename)
{
    int length = 0;
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644); // Open or create the file with appropriate permissions
    char *mtime_str = NULL;
    char buffer[PATH_MAX + 100]; // Temporary buffer for formatted strings

    if (fd == -1)
    {
        perror("open");
        return;
    }

    for (int i = 0; i < result_count; ++i)
    {
        // 맨 끝 개행문자 제거
        mtime_str = ctime(&results[i].mtime);
        mtime_str[strcspn(mtime_str, "\n")] = '\0';

        length = snprintf(buffer, sizeof(buffer), "%s (Size: %lu bytes, Modified: %s)\n", results[i].path, results[i].size, mtime_str);

        if (write(fd, buffer, length) == -1)
        {
            perror("write");
            close(fd);
            return;
        }
    }

    close(fd);
    printf("Results saved to '%s'\n", filename);
}

void draw_input_box(WINDOW *win, int y, int x, const char *label) {
    box(win, 0, 0);
    mvwprintw(win, 0, 1, "%s", label);
    wrefresh(win);
}

void search_files() {
    char input[256];
    char directory[PATH_MAX];
    char absolute_path[PATH_MAX];
    char save_option = 0;
    char save_file_name[PATH_MAX];
    size_t min_size, max_size;
    char min_mtime_str[32], max_mtime_str[32];

    initscr();
    cbreak();
    echo();
    
    memset(input, 0, sizeof(input));
    memset(directory, 0, sizeof(directory));

    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    WINDOW *win = newwin(5, cols - 4, (rows - 5) / 2, 2);

    // 디렉토리 입력 받기
    clear();
    draw_input_box(win, 1, 1, " Enter absolute directory path to search ");
    mvwgetstr(win, 2, 1, directory);
    wclear(win);
    wrefresh(win);
    if (realpath(directory, absolute_path) == NULL) {
        clear();
        printw("Error: Cannot resolve path '%s'.\n", directory);
        printw("Press any key to continue\n");
        refresh();
        wgetch(win);
        endwin();
        return;
    }

    memset(input, 0, sizeof(input));

    // 최소 파일 크기 입력 받기
    clear();
    draw_input_box(win, 1, 1, " Enter minimum file size (in bytes) ");
    mvwgetstr(win, 2, 1, input);
    wclear(win);
    wrefresh(win);
    char *endptr;
    min_size = strtoul(input, &endptr, 10);
    if (*endptr != '\0') {
        clear();
        printw("Error: Minimum size must be a number.\n");
        printw("Press any key to continue\n");
        refresh();
        wgetch(win);
        endwin();
        return;
    }

    memset(input, 0, sizeof(input));

    // 최대 파일 크기 입력 받기
    clear();
    draw_input_box(win, 1, 1, " Enter maximum file size (in bytes) ");
    mvwgetstr(win, 2, 1, input);
    wclear(win);
    wrefresh(win);
    max_size = strtoul(input, &endptr, 10);
    if (*endptr != '\0') {
        clear();
        printw("Error: Maximum size must be a number.\n");
        printw("Press any key to continue\n");
        refresh();
        wgetch(win);
        endwin();
        return;
    }

    memset(input, 0, sizeof(input));

    // 최소 수정 날짜 입력 받기
    clear();
    draw_input_box(win, 1, 1, " Enter minimum modification date (YYYY-MM-DD) ");
    mvwgetstr(win, 2, 1, min_mtime_str);
    wclear(win);
    wrefresh(win);

    memset(input, 0, sizeof(input));

    // 최대 수정 날짜 입력 받기
    clear();
    draw_input_box(win, 1, 1, " Enter maximum modification date (YYYY-MM-DD) ");
    mvwgetstr(win, 2, 1, max_mtime_str);
    wclear(win);
    wrefresh(win);

    memset(input, 0, sizeof(input));

    // 저장 여부 입력 받기
    clear();
    draw_input_box(win, 1, 1, " Do you want to save the results to a file? (y for yes, n for no) ");
    mvwgetstr(win, 2, 1, input);
    wclear(win);
    wrefresh(win);
    save_option = input[0];
    if (save_option != 'n' && save_option != 'y') {
        clear();
        printw("Error: Invalid input for saving option.\n");
        printw("Press any key to continue\n");
        refresh();
        wgetch(win);
        endwin();
        return;
    }

    memset(input, 0, sizeof(input));

    if (save_option == 'y') {
        // 저장할 파일 이름 입력 받기
        clear();
        draw_input_box(win, 1, 1, "Enter file name to save results: ");
        printw("Press any key to continue\n");
        mvwgetstr(win, 2, 1, save_file_name);
        wclear(win);
        wrefresh(win);
    }

    struct tm tm;
    time_t min_mtime, max_mtime;

    memset(&tm, 0, sizeof(struct tm));
    if (strptime(min_mtime_str, "%Y-%m-%d", &tm) == NULL) {
        clear();
        printw("Error: Invalid date format for min_mtime. Use YYYY-MM-DD.\n");
        printw("Press any key to continue\n");
        refresh();
        wgetch(win);
        endwin();
        return;
    }
    min_mtime = mktime(&tm);
    if (min_mtime == -1) {
        clear();
        printw("Error: Failed to convert min_mtime to time_t.\n");
        printw("Press any key to continue\n");
        refresh();
        wgetch(win);
        endwin();
        return;
    }

    memset(&tm, 0, sizeof(struct tm));
    if (strptime(max_mtime_str, "%Y-%m-%d", &tm) == NULL) {
        clear();
        printw("Error: Invalid date format for max_mtime. Use YYYY-MM-DD.\n");
        printw("Press any key to continue\n");
        refresh();
        wgetch(win);
        endwin();
        return;
    }
    max_mtime = mktime(&tm);
    if (max_mtime == -1) {
        clear();
        printw("Error: Failed to convert max_mtime to time_t.\n");
        printw("Press any key to continue\n");
        refresh();
        wgetch(win);
        endwin();
        return;
    }

    // 디렉토리 유효성 검사
    struct stat dir_stat;
    if (stat(directory, &dir_stat) != 0 || !S_ISDIR(dir_stat.st_mode)) {
        clear();
        printw("Error: '%s' is not a valid directory. (%s)\n", directory, strerror(errno));
        printw("Press any key to continue\n");
        refresh();
        wgetch(win);
        endwin();
        return;
    }

    clock_t start_time = clock();

    clear();
    mvprintw(0, 0, "Searching '%s'...\n", absolute_path);
    refresh();
    search_directory(absolute_path, min_size, max_size, min_mtime, max_mtime);

    clock_t end_time = clock();
    double elapsed_time = calculate_elapsed_time(start_time, end_time);
    clear();
    mvprintw(0, 0, "Search completed in %.2f seconds.\n", elapsed_time);
    refresh();

    qsort(results, result_count, sizeof(FileInfo), compare_by_size);

    getmaxyx(stdscr, rows, cols);

    // 결과 페이지네이션
    WINDOW *win2 = newwin(rows - 2, cols - 2, 1, 1);
    paginate_results(win2);

    if (save_option == 'y') {
        save_results(save_file_name);
    }

    endwin();
}

