void coroutines_initialize();
void coroutines_gather();
void coroutine_add(void (*func) (void), const int argcount, void* arg);
void coroutine_yield();
void coroutine_sleep(unsigned int seconds);
