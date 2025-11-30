#include <stdio.h>
void coroutines_initialize();
void coroutines_gather();
void coroutine_add(void (*func) (void), const int argcount, void* arg);
void coroutine_yield();
void coroutine_sleep(unsigned int seconds);
ssize_t coroutine_read(int fd, void* buf, size_t bufSize);
ssize_t coroutine_write(int fd, const char* buf, size_t bufSize);
void coroutines_cleanup();
