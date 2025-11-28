lib_test: main.o timeheap.o
	gcc main.o timeheap.o -o t
	./t
main.o:
	gcc -g -c main.c
timeheap.o:
	gcc -g -c timeheap.c
clean:
	rm timeheap.o main.o t
