VPATH = ../../
all: client echoserver

client: client.o timeheap.o coroutines.o
	gcc client.o timeheap.o coroutines.o -o client
echoserver: echoserver.o timeheap.o coroutines.o
	gcc echoserver.o timeheap.o coroutines.o -o echoserver
%.o: %.c
	gcc -g -c $<
clean:
	rm *.o client echoserver
