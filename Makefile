CC = gcc
CFLAGS = -Wall -pthread
OBJ = server.o cq.o
TARGET = server

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

server.o: server.c cq.h
	$(CC) $(CFLAGS) -c server.c

cq.o: cq.c cq.h
	$(CC) $(CFLAGS) -c cq.c

clean:
	rm -f *.o $(TARGET)
