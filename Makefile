CC = gcc
CFLAGS = -O3 -Wall -Wextra
TARGET = temp_monitor

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) main.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)