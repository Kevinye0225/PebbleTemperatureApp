
all: server2

server2: server2.c
	clang -o server2 server2.c -lpthread

clean:
	rm -rf main *.o
