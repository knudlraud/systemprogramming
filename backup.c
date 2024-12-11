/**************************************************************
 * backup.c
 * 2023014973 노성민, 2023004915 장시훈
 * 2024 fall semester sysytem programming project.
 * Backing up and monitoring with visualize part of project.
 *************************************************************/


#include "backup.h"

int PAGELEN = 0;
int LINELEN = 0;

int CYCLE = 1; // 1 minutes for the backup cycle
int isbackup = 1;
pthread_t backup_thread;
char main_source_dir[PATH_MAX];
WINDOW* win;

int is_modified(const char* saved_path, const char* now_path) {
	struct stat saved_stat, now_stat;
	if (stat(saved_path, &saved_stat) == -1) return 1; //backupped
	if (stat(now_path, &now_stat) == -1) return 1;
	return saved_stat.st_mtime > now_stat.st_mtime; // compare edited time
}

int copy_file(const char* source, const char* destination) {

	FILE* src = fopen(source, "rb");
	if (src == NULL) {
		mvwprintw(win, PAGELEN - 2, 2, "Failed to open source file", strerror(errno));
		return -1;
	}
	FILE* dst = fopen(destination, "wb"); // backup folder

	if (dst == NULL) {
		mvwprintw(win, PAGELEN - 2, 2, "Failed to open destination file", strerror(errno));
		fclose(src);  // src가 열렸으면 닫아줘야 함
		return -1;
	}
	char buffer[1024];
	size_t bytes;

	while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
		fwrite(buffer, 1, bytes, dst);
	}

	fclose(src);
	fclose(dst);
	return 1;
}

void get_time(time_t t, char* buffer, size_t buffer_size) {
	struct tm* local_time = localtime(&t);
	if (local_time == NULL) {
		mvwprintw(win, PAGELEN - 2, 2, "Failed to convert to local time", strerror(errno));
		return;
	}

	strftime(buffer, buffer_size, "_%Y.%m.%d_%Hh%Mm%Ss", local_time);

}

void change_filename(const char* dest_path) {
	struct stat st;
	char buffer[100];
	char name[PATH_MAX];
	char extension[PATH_MAX];
	if (stat(dest_path, &st) == -1) return;
	get_time(st.st_mtime, buffer, sizeof(buffer));

	char new_dest_path[PATH_MAX];
	char* dot = strrchr(dest_path, '.');
	if (dot != NULL) {
		strncpy(name, dest_path, dot - dest_path);
		name[dot - dest_path] = '\0';
		strcpy(extension, dot + 1);
		snprintf(new_dest_path, sizeof(new_dest_path), "%s%s.%s", name, buffer, extension);
	}
	else {
		snprintf(new_dest_path, sizeof(new_dest_path), "%s%s", dest_path, buffer);
	}
	rename(dest_path, new_dest_path);
}

void perform_incremental_backup(const char* source) {
	DIR* dir = opendir(source);
	if (dir == NULL) {
		wclear(win);
		endwin();
		perror("\nFail to open source directory\n");
		exit(-1);
	}
	struct dirent* entry;

	while ((entry = readdir(dir)) != NULL) {

		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
		if (strstr(entry->d_name, "_backup") != NULL) {
			continue;
		}
		char source_path[512], backup_dir[PATH_MAX], dest_path[512];
		snprintf(source_path, sizeof(source_path), "%s/%s", source, entry->d_name);
		snprintf(backup_dir, sizeof(dest_path), "%s/%s_backup", source, entry->d_name);
		//
		struct stat entry_stat;
		if (stat(source_path, &entry_stat) == -1) {
			mvwprintw(win, PAGELEN - 2, 2, "Failed to get file statues", strerror(errno));
			continue;
		}

		if (S_ISREG(entry_stat.st_mode)) {
			// 파일에 대해 `_backup` 디렉토리 생성
			if (mkdir(backup_dir, 0755) == -1 && errno != EEXIST) {
				mvwprintw(win, PAGELEN - 2, 2, "Failed to create backup directory", strerror(errno));
				continue;
			}
			//
			snprintf(dest_path, sizeof(dest_path), "%s/%s", backup_dir, entry->d_name);

			if (is_modified(source_path, dest_path)) {
				change_filename(dest_path);
				if (copy_file(source_path, dest_path) < 0)
					printf("Failed to backup file: %s\n", source_path);
			}
		}
		else if (S_ISDIR(entry_stat.st_mode)) {
			// 하위 디렉토리 탐색
			perform_incremental_backup(source_path);
		}
	}

	closedir(dir);
}

int see_more(FILE* cmd)
{
	int c = 0;
	printf("\033[7m more? \033[m"); //more?
	while ((c = getc(cmd)) != EOF) { // char by char
		if (c == 'q')
			return 0;
		if (c == ' ')
			return PAGELEN - 1;
	}
	return 0;
}

void do_more(FILE* fp)
{
	WINDOW* win2 = newwin(PAGELEN, LINELEN, 0, 0);
	char line[LINELEN];
	int num_of_line = 0;
	int reply = 0;
	FILE* fp_tty = fopen("/dev/tty", "r");
	if (fp_tty == NULL) // file is empty
		exit(1);

	while (fgets(line, LINELEN, fp) != NULL) // from fp save chars in line which length is LINELEN
	{
		if (num_of_line == PAGELEN - 1) { // so long that need 'more'
			wrefresh(win2);
			wclear(win2);
			reply = see_more(fp_tty);
			if (reply == 0)
				break;
			num_of_line -= reply;
		}
		mvwprintw(win2, num_of_line, 2, "%s", line);

		num_of_line++;
	}
	wrefresh(win2);
	wgetch(win2);
	delwin(win2);
}

void view_backuplists(const char* backup_dir) {
	char bd[PATH_MAX];
	strcpy(bd, backup_dir);
	char buffer[PATH_MAX];
	struct dirent* entry;
	int i = 3;
	DIR* dir = opendir(dirname(bd));
	if (dir == NULL) {
		mvwprintw(win, 9, 2, "file does not exist.");
		wrefresh(win);
		wgetch(win);
		return;
	}
	closedir(dir);
	snprintf(buffer, sizeof(buffer), "%s_backup", backup_dir);
	dir = opendir(buffer);
	if (dir == NULL) {
		mvwprintw(win, 9, 2, "There's no backup files.");
		wrefresh(win);
		wgetch(win);;
		return;
	}
	const char* last_slash = strrchr(backup_dir, '/');
	while ((entry = readdir(dir)) != NULL) {

		// Skip "." and ".."
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		//if (strstr(entry->d_name, last_slash + 1) != NULL) {
		mvwprintw(win, i, 2, "- %s", entry->d_name);
		i++;
		//}
	}
	wrefresh(win);
	i++;
	int a = i;
	while (1) {
		i = a;
		int addictional_input = 0;
		mvwprintw(win, i, 2, "==Select additional actions==="); i++;
		mvwprintw(win, i, 2, "1. Preview a backup file"); i++;
		mvwprintw(win, i, 2, "2. Change to a backup file"); i++;
		mvwprintw(win, i, 2, "3. Empty entire backup file"); i++;
		mvwprintw(win, i, 2, "4. Back to the Backup Menu"); i++;
		mvwprintw(win, i, 2, "==============================="); i++;
		mvwprintw(win, i, 2, "Enter the menu: ");
		wrefresh(win);
		wscanw(win, "%d", &addictional_input);
		switch (addictional_input) {
		case 1: {
			FILE* fp;

			char filepath[PATH_MAX];
			char change_file[PATH_MAX];
			FILE* f;

			mvwprintw(win, i, 2, "Enter the NAME of the backup file. ");
			wscanw(win, "%s", change_file);
			snprintf(filepath, sizeof(filepath), "%s/%s", buffer, change_file);
			if ((f = fopen(filepath, "r")) != NULL) {
				do_more(f);
				fclose(f);
			}
			else {
				mvwprintw(win, 1 + i, 2, "Wrong name.");
				wrefresh(win);
				wgetch(win);
			}
			wclear(win);
			box(win, 0, 0);
			break;
			break;
		}
		case 2: {
			char filepath[PATH_MAX];
			char change_file[PATH_MAX];
			mvwprintw(win, 1 + i, 2, "Enter the NAME of the backup file. ");
			wscanw(win, "%s", change_file);
			snprintf(filepath, sizeof(filepath), "%s/%s", buffer, change_file);
			if (copy_file(filepath, backup_dir) > 0) { mvwprintw(win, 2 + i, 2, "Complete to change file!"); wrefresh(win); wgetch(win); }
			else { wrefresh(win); wgetch(win); }
			wclear(win);
			box(win, 0, 0);
			break;
		}
		case 3: {
			rewinddir(dir);
			while ((entry = readdir(dir)) != NULL) {
				char filepath[PATH_MAX];
				// Skip "." and ".."
				if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
					continue;
				snprintf(filepath, sizeof(filepath), "%s/%s", buffer, entry->d_name);
				//There's warning but we'll ignore in 148

				//if (strstr(entry->d_name, last_slash + 1) != NULL) {
				remove(filepath);
				//}
			}
			mvwprintw(win, i + 1, 2, "Deletion Completed");
			wrefresh(win);
			wgetch(win);
			closedir(dir);
			return;
		}
		case 4: closedir(dir); return;
		default: {
			mvwprintw(win, i + 1, 2, "Unsupported number");
			wrefresh(win);
			wgetch(win);
			wclear(win);
			box(win, 0, 0); break;
		}
		}
	}

}

void cycle_backup(const char* source) {
	while (isbackup) {
		perform_incremental_backup(source);
		sleep(CYCLE);
	}
}

void* cycle_backup_thread(void* arg) {
	char* source_dir = (char*)arg;
	cycle_backup(source_dir);
	return NULL;
}

void set_isbackup() {
	isbackup = !isbackup;

	if (isbackup) {
		mvwprintw(win, 9, 2, "Backupping is activated!");
		if (pthread_create(&backup_thread, NULL, cycle_backup_thread, (void*)main_source_dir) != 0) {
			mvwprintw(win, PAGELEN - 2, 2, "Failed to create backup thread", strerror(errno));
			exit(-1);
		}
	}
	else mvwprintw(win, 9, 2, "Backupping is deactivated!");
	wrefresh(win);
	wgetch(win);
}

int backup_main() {
	struct winsize wbuf;

	if (ioctl(0, TIOCGWINSZ, &wbuf) != -1) {
		PAGELEN = wbuf.ws_row;
		LINELEN = wbuf.ws_col;
	}

	initscr();
	echo();
	cbreak();
	curs_set(0);

	win = newwin(PAGELEN, LINELEN, 0, 0);
	box(win, 0, 0);

	mvwprintw(win, 1, 2, "Enter the source directory to backup: ");
	wrefresh(win);
	wgetnstr(win, main_source_dir, 255);
	wclear(win);

	DIR* dir = opendir(main_source_dir);
	if (dir == NULL) {
		wclear(win);
		box(win, 0, 0);
		mvwprintw(win, 1, 4, "Fail to open source directory", strerror(errno));
		wgetch(win);
		wclear(win);
		endwin();
		closedir(dir);
		return -1;
	}

	pthread_create(&backup_thread, NULL, cycle_backup_thread, (void*)main_source_dir);


	while (1) {
		int input = 0;
		mvwprintw(win, 1, 2, "=====Backup Menu=====");
		mvwprintw(win, 2, 2, "1. View list of backupfiles.");
		mvwprintw(win, 3, 2, "2. Stop or Start backupping");
		mvwprintw(win, 4, 2, "3. Back to the Main Menu");
		mvwprintw(win, 5, 2, "=====================");
		mvwprintw(win, 6, 2, "Enter the menu: ");
		box(win, 0, 0);
		wscanw(win, "%d", &input);
		wrefresh(win);
		switch (input) {
		case 1: {
			wclear(win);
			box(win, 0, 0);
			char input_dir[PATH_MAX];
			mvwprintw(win, 1, 2, "Enter the file path to check. ");
			wscanw(win, "%s", input_dir);
			view_backuplists(input_dir);
			wclear(win);
			break;
		}

		case 2: {
			char input5;
			while (1) {
				if (isbackup) {
					mvwprintw(win, 8, 2, "Backup is currently activated. Do you want to deactivate it? (y/n) ");
				}
				else {
					mvwprintw(win, 8, 2, "Backup is currently deactivated. Do you want to activate it? (y/n) ");
				}

				wscanw(win, " %c", &input5);
				if (input5 == 'y') {
					set_isbackup();
					wclear(win);

					break;
				}
				else if (input5 == 'n') { wclear(win); break; }
				else {
					mvwprintw(win, 9, 2, "Unsupported input.");
					wrefresh(win);
					wgetch(win);
					wclear(win);
					break;
				}
			}
			break;
		}
		case 3: wclear(win); endwin(); return 1;
		default: {
			mvwprintw(win, 9, 2, "Unsupported input.");
			wrefresh(win);
			wgetch(win);
			wclear(win);
			break;
		}
		}
	}
	return -1;

}
