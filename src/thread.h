#ifndef __OBSCURA_THREAD_H__
#define __OBSCURA_THREAD_H__ 1

#include <pthread.h>
#include <stdint.h>

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void	(*PFN_ObscuraExecutionFunction)	(void *);

typedef struct __thread_pool_task {
	PFN_ObscuraExecutionFunction	*func;
	void				*arg;
} __thread_pool_task_t;

typedef struct __thread_pool_worker {
	pthread_t	thread;
} __thread_pool_worker_t;

typedef struct ObscuraThreadPool {
	uint8_t			 workers_capacity;
	uint8_t			 workers_idle	__attribute__((aligned(LEVEL1_DCACHE_LINESIZE)));
	__thread_pool_worker_t	*workers;

	uint32_t		 tasks_capacity;
	uint32_t		 tasks_waiting	__attribute__((aligned(LEVEL1_DCACHE_LINESIZE)));
	__thread_pool_task_t	*tasks;
} ObscuraThreadPool;

extern ObscuraThreadPool *	ObscuraCreateThreadPool		(uint8_t, uint32_t, ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyThreadPool	(ObscuraThreadPool **, ObscuraAllocationCallbacks *);

extern void	ObscuraEnqueueWork	(ObscuraThreadPool *, PFN_ObscuraExecutionFunction, void *);
extern void	ObscuraWaitAll		(ObscuraThreadPool *);

#ifdef __cplusplus
}
#endif

#endif
