CC = gcc
LIBS = -lncursesw -lpthread
CFLAGS = -Wall -Wextra
OBJS = main.o search_files.o storage_analysis.o monibackup.o
TARGET = file_explorer.out

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

main.o: search_files.h storage_analysis.h main.c
	$(CC) $(CFLAGS) -c main.c

monibackup.o: monibackup.h monibackup.c
	$(CC) $(CFLAGS) -c monibackup.c

search_files.o: search_files.h search_files.c
	$(CC) $(CFLAGS) -c search_files.c

storage_analysis.o: storage_analysis.h storage_analysis.c
	$(CC) $(CFLAGS) -c storage_analysis.c

clean:
	rm -f $(OBJS) $(TARGET)