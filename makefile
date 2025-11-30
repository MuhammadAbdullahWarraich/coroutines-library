VPATH = tests/e2e/t1:tests/e2e/t2:tests/e2e/t3:tests/timeheap-tests/
t1: coroutines.o test_yield.o timeheap.o
	gcc coroutines.o timeheap.o test_yield.o -o t1
	./t1
t2: coroutines.o test_sleep.o timeheap.o
	gcc coroutines.o timeheap.o test_sleep.o -o t2
	./t2
t3: coroutines.o test_rw.o timeheap.o
	gcc coroutines.o timeheap.o test_rw.o -o t3
	./t3
timeheap_tests: th_test1.o th_test2.o timeheap.o
	gcc th_test1.o timeheap.o -o th_test1
	./th_test1
	gcc th_test2.o timeheap.o -o th_test2
	./th_test2
	rm th_test1 th_test2 *.o
%.o: %.c
	gcc -g -c $<
clean:
	rm *.o t1 t2 t3
