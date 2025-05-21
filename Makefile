
CC=gcc

CFLAGS=-Wall -std=c99

TARGET=mySystemStats

all: $(TARGET)

$(TARGET): mySystemStats.o stats_functions.o
	$(CC) $(CFLAGS) -o $(TARGET) mySystemStats.o stats_functions.o

mySystemStats.o: mySystemStats.c
	$(CC) $(CFLAGS) -c mySystemStats.c

stats_functions.o: stats_functions.c
	$(CC) $(CFLAGS) -c stats_functions.c

clean:
	rm -f $(TARGET) *.o
