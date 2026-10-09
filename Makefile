CC = gcc
CFLAGS = -Wall -Wextra

httpServer: http.o
	$(CC) http.o -o httpServer

http.o: http.c
	$(CC) -c http.c $(CFLAGS) 

.PHONY: run
run: httpServer
	./httpServer

.PHONY: clean
clean:
	rm -f *.o httpServer