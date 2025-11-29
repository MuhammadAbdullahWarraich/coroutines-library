VPATH = tests/t1:tests/t2
t1: coroutines.o test_yield.o timeheap.o
	gcc coroutines.o timeheap.o test_yield.o -o t1
	./t1
t2: coroutines.o test_sleep.o timeheap.o
	gcc coroutines.o timeheap.o test_sleep.o -o t2
	./t2
%.o: %.c
	gcc -g -c $<
clean:
	rm *.o t1 t2
