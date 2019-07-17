#ifndef __OBSCURA_THREAD_H__
#define __OBSCURA_THREAD_H__ 1

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void *	(*PFN_ObscuraTaskFunction)	(void *);

struct __work_queue_task {
	PFN_ObscuraTaskFunction	 func;
	void			*arg;
};

struct __work_queue_thread {
	pthread_t	thread;
	uint64_t	cursor;
};

typedef void	(*PFN_ObscuraWaitStrategy)	(void);

extern void	ObscuraBusySpinWait	(void);
extern void	ObscuraYieldWait	(void);

typedef struct ObscuraWorkQueue {
	uint32_t			 threads_capacity;
	struct __work_queue_thread	*threads;

	uint32_t			 tasks_capacity;
	struct __work_queue_task	*tasks;

	PFN_ObscuraWaitStrategy	wait_strategy;

	volatile uint64_t	tasks_head_cursor	__attribute__((aligned(LEVEL1_DCACHE_LINESIZE)));
	volatile uint64_t	tasks_tail_cursor	__attribute__((aligned(LEVEL1_DCACHE_LINESIZE)));
	volatile uint64_t	tasks_consumer_cursor	__attribute__((aligned(LEVEL1_DCACHE_LINESIZE)));

	bool	running;
} ObscuraWorkQueue;

extern ObscuraWorkQueue *	ObscuraCreateWorkQueue	(uint32_t, uint32_t, PFN_ObscuraWaitStrategy, ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyWorkQueue	(ObscuraWorkQueue **, ObscuraAllocationCallbacks *);

extern void	ObscuraEnqueueTask	(ObscuraWorkQueue *, PFN_ObscuraTaskFunction, void *);
extern void	ObscuraWaitAll		(ObscuraWorkQueue *);

#ifdef __cplusplus
}
#endif

#endif
