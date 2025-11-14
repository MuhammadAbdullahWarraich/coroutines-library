
/*void append(Contexts* contexts, ucontext_t* c) {
	//TODO: change this to a macro
	//TODO: put malloc error handling in a macro as well
	if (0 == capacity) {
		contexts.arr = (ucontext_t**) malloc(sizeof(ucontext_t*) * 10);
		if (NULL == contexts.arr) {
			printf("malloc failed to allocate %zu bytes\n", 10 * sizeof(ucontext_t*));
			exit(1);
		}
		contexts.arr[contexts.size] = c;
		contexts.size++;
		return;
	}
	if (capacity == size) {
		ucontext** t_arr = (ucontext_t**) malloc(2 * contexts.capacity * sizeof(ucontext_t*));
		if (NULL == t_arr) {
			printf("malloc failed to allocate %zu bytes\n", 2 * contexts.capacity * sizeof(ucontext_t*));
			exit(1);
		}
		for (size_t i = 0; i < contexts.size; i++) t_arr[i] = contexts.arr[i];
		t_arr[contexts.size] = c;
		contexts.size++;
		contexts.capacity = 2 * contexts.capacity;
		contexts.arr = t_arr;
		return;
	}
	contexts.arr[contexts.size] = c;
	contexts.size++;
}*/
