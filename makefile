VPATH = tests/e2e/t1:tests/e2e/t2:tests/e2e/t3:tests/e2e/t4:tests/timeheap-tests/
lib: coroutines.o timeheap.o
	ar -cvq libcoroutines.a coroutines.o timeheap.o
t1: lib test_yield.o
	gcc -o t1 test_yield.o -L./ -lcoroutines
	./t1
t2: lib test_sleep.o
	gcc -o t2 test_sleep.o -L./ -lcoroutines
	./t2
t3: lib test_rw.o
	gcc -o t3 test_rw.o -L./ -lcoroutines
	./t3
t4: lib test_await.o
	gcc -o t4 test_await.o -L./ -lcoroutines
	./t4
timeheap_tests: timeheap.o th_test1.o th_test2.o
	gcc th_test1.o timeheap.o -o th_test1
	./th_test1
	gcc th_test2.o timeheap.o -o th_test2
	./th_test2
	rm th_test1 th_test2 *.o
%.o: %.c
	gcc -g -c $<
clean:
	rm *.o *.a t1 t2 t3 t4
