# Coroutines Library in C

A lightweight **coroutines library** in C providing both **stackful** and **stackless** coroutine implementations. Designed for asynchronous programming with features like yielding, sleeping, and asynchronous I/O operations.

## Features

- **Stackful Coroutines** using `ucontext.h`
  - Yield execution between coroutines
  - Sleep functionality
  - Async read and write operations
- **Planned Features**
  - Stackful coroutines using inline assembly
  - Stackless coroutines using callbacks
  - Automated test orchestrator
  - Dockerized library build

## Installation

Clone the repository:

```bash
git clone https://github.com/MuhammadAbdullahWarraich/coroutines-library.git
cd coroutines-library
````

Build using the provided Makefile:

```bash
make
```

> Note: The Makefile currently compiles tests and examples. Future versions will support building a static/shared library.

## Usage

Include the libcoroutines.a file in your project directory(or maybe /usr/lib), include the header file in your project, and include the following directive in your C project:

```c
#include "coroutines.h"
```

Initialize coroutines runtime, add coroutines, and then wait for them to complete:

```c
#include "coroutines.h"
struct CounterArgs {
        int id;
        int n;
};
typedef struct CounterArgs CounterArgs;
void counter(void* args) {
        CounterArgs* cargs = (CounterArgs*) args;
        int id = cargs->id;
        int n = cargs->n;
        for (int i = 0; i < n; i++) {
                printf("id: %d ; i: %d\n", id, i);
                coroutine_yield();
        }
        printf("returning from %d\n", id);
}
int main() {
        coroutines_initialize();
        CounterArgs c1_args = {
                .id = 1,
                .n = 2
        };
        CounterArgs c2_args = {
                .id = 2,
                .n = 5
        };
        CounterArgs c3_args = {
                .id = 3,
                .n = 5
        };
        coroutine_add((void (*) (void)) counter, 1, (void *) &c1_args);
        coroutine_add((void (*) (void)) counter, 1, (void *) &c2_args);
        coroutine_add((void (*) (void)) counter, 1, (void *) &c3_args);
        coroutines_gather();
        return 0;
}
```

Refer to the `examples/` directory for full client-server usage and test cases. Refer to the `tests/e2e/` directory for more example programs.

## API Overview

* `void coroutines_initialize();`: Initializes the coroutine system.
* `void coroutine_add(void (*func) (void), const int argcount, void* arg);`: Creates a new coroutine. `argcount` can be either 0(`arg` is `NULL`) or 1(for multiple arguments, use structs(see above example)).
* `void coroutine_yield();`: Suspends the current coroutine and resumes another.
* `void coroutine_sleep(unsigned int seconds);`: Pauses the coroutine for the specified seconds.
* `ssize_t coroutine_read(int fd, void* buf, size_t bufSize);`: Non-blocking read.
* `ssize_t coroutine_write(int fd, const char* buf, size_t bufSize);`: Non-blocking write.
* `void coroutines_gather();`: busy waits until all coroutines have ended
* `void coroutines_cleanup();`: Cleanup memory allocated by the coroutine system. For using coroutines after this function, call `coroutines_initialize()` again.

## Examples

- Check the `examples/` folder for:
	* Client-server asynchronous communication
- Check the `tests/e2e/` folder for more examples.

## Performance Results

> **TODO:** Benchmark the library against traditional threading and other coroutine libraries. Include memory usage, context switch time, and latency.

## Contributing

Contributions, bug reports, and feature requests are welcome! Please open an issue or submit a pull request.

## License

MIT License. See `LICENSE` for details.

## References

* https://sourceware.org/glibc/manual/2.42/html_node/System-V-contexts.html
* https://www.ibm.com/docs/en/zos/3.1.0?topic=functions-makecontext-modify-user-context
* https://stackoverflow.com/questions/67868071/how-to-know-which-version-of-c-language-i-am-using
* https://github.com/Apple-FOSS-Mirror/Libc/blob/2ca2ae74647714acfc18674c3114b1a5d3325d7d/x86_64/gen/makecontext.c#L98-L107

## Todos
- stackful coroutines using ucontext.h
	- make a automated test orchestrator that executes and validates all tests automatically with summary
- stackful coroutines using inline assembly
	- all features
- stackless coroutines using callbacks
	- all features

