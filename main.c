/**************************************************************
 * main.c
 * 2023006811 전하준
 * 2024 fall semester sysytem programming project.
 * Main function that connects each function.
 *************************************************************/

#include <locale.h>
#include <ncurses.h>
#include <string.h>
#include <signal.h>
#include "storage_analysis.h"
#include "search_files.h"
#include "backup.h"
#include "moni.h"//moni헤더

int main() {
    setlocale(LC_ALL, "ko_KR.utf8");

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
        napms(100);
        wclear(win);
        box(win, 0, 0);

        // 터미널 크기 가져오기
        int height, width;
        getmaxyx(win, height, width);

        // "고급 파일 탐색기" 텍스트의 위치 계산
        int text_y = height / 4;  // 화면 높이의 1/4 지점 (중간 위쪽)
        int text_x = (width - strlen("고급 파일 탐색기")) / 2;  // 가운데 정렬

        mvwprintw(win, text_y, text_x, "고급 파일 탐색기");

        // 1., 2., 3., 4. 항목 추가
        const char *items[] = {"1. 스토리지 사용량", "2. 파일 검색", "3. 백업", "4. 모니터링", "5. 종료"};//4번5번수정
        int num_items = sizeof(items) / sizeof(items[0]);

        for (int i = 0; i < num_items; i++) {
            mvwprintw(win, text_y + (height/5) + (2*i), text_x, "%s", items[i]);
        }

        echo();
        noecho();

        int ch = wgetch(win);
        if (ch == '5') quit = 1;
        if (ch == '1'){
            wclear(win);
            wrefresh(win);
            storage_analysis_main();
        }
        if (ch == '2'){
            wclear(win);
            wrefresh(win);
            search_files();
        }
        if (ch == '3'){
            wclear(win);
            wrefresh(win);
            backup_main();
        }
        if (ch == '4'){ // 여기서 모니터링으로 변경
            wclear(win);
            wrefresh(win);
            moni_main(); // 모니터링 함수 호출
        }
        wrefresh(win);
    } while (!quit);

    // Cleanup ncurses
    delwin(win);
    endwin();

    return 0;
}
