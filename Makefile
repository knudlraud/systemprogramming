CC = gcc
LIBS = -lncursesw -lpthread
CFLAGS = -Wall -Wextra
OBJS = main.o search_files.o storage_analysis.o backup.o moni.o
TARGET = file_explorer.out

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(OBJS) $(LIBS)

main.o: search_files.h storage_analysis.h main.c
	$(CC) $(CFLAGS) -c main.c

backup.o: backup.h backup.c
	$(CC) $(CFLAGS) -c backup.c

moni.o: moni.h moni.c
	$(CC) $(CFLAGS) -c moni.c

search_files.o: search_files.h search_files.c
	$(CC) $(CFLAGS) -c search_files.c

storage_analysis.o: storage_analysis.h storage_analysis.c
	$(CC) $(CFLAGS) -c storage_analysis.c

clean:
	rm -f $(OBJS) $(TARGET)
