#include "thread.h"

ObscuraThreadPool *ObscuraCreateThreadPool(uint8_t workers_capacity, uint32_t tasks_capacity, ObscuraAllocationCallbacks *allocator) {
	ObscuraThreadPool *thpool = allocator->allocation(sizeof(ObscuraThreadPool), 8);

	thpool->tasks_capacity = tasks_capacity;
	thpool->tasks = allocator->allocation(sizeof(__thread_pool_task_t) * thpool->tasks_capacity, PAGESIZE);

	thpool->workers_capacity = workers_capacity;
	thpool->workers = allocator->allocation(sizeof(__thread_pool_worker_t) * thpool->workers_capacity, PAGESIZE);

	return thpool;
}

void ObscuraDestroyThreadPool(ObscuraThreadPool **ptr, ObscuraAllocationCallbacks *allocator) {
	allocator->free((*ptr)->workers);
	allocator->free((*ptr)->tasks);
	allocator->free(*ptr);

	*ptr = NULL;
}

void ObscuraEnqueueWork(ObscuraThreadPool *thpool, PFN_ObscuraExecutionFunction func, void *arg) {
}

void ObscuraWaitAll(ObscuraThreadPool *thpool) {
}
