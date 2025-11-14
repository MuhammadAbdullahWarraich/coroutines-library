# coroutines-library
Coroutines implementations(stackful &amp; stackless) in C
## Helpful Links
https://sourceware.org/glibc/manual/2.42/html_node/System-V-contexts.html
https://www.ibm.com/docs/en/zos/3.1.0?topic=functions-makecontext-modify-user-context
https://stackoverflow.com/questions/67868071/how-to-know-which-version-of-c-language-i-am-using
https://github.com/Apple-FOSS-Mirror/Libc/blob/2ca2ae74647714acfc18674c3114b1a5d3325d7d/x86_64/gen/makecontext.c#L98-L107
## Done
- stackful coroutines using ucontext.h
	- coroutines using yield
## Todo
- stackful coroutines using ucontext.h
	- remove memory leakage
	- make the code clean
	- implement coroutine_read, coroutine_write, coroutine_sleep
- stackful coroutines using inline assembly
	- all features
- stackless coroutines using callbacks
	- all features
- clean up codebase(make header files, refactor code, add tests suite, dockerize the application)
