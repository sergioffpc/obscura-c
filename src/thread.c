#include <assert.h>
#include <emmintrin.h>
#include <errno.h>
#include <limits.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "thread.h"

static void *
start_routine(void *arg)
{
	ObscuraWorkQueue *wq = arg;

	struct __work_queue_thread *thr = NULL;
	for (uint32_t i = 0; i < wq->threads_capacity; i++) {
		if (wq->threads[i].thread == pthread_self()) {
			thr = &wq->threads[i];
			break;
		}
	}

	while (wq->running) {
		thr->cursor = __sync_fetch_and_add(&wq->tasks_consumer_cursor, 1);
		while (thr->cursor >= wq->tasks_head_cursor) {
			if (!wq->running) {
				break;
			}

			wq->wait_strategy();
		}

		__sync_fetch_and_sub(&wq->idle_count, 1);

		uint32_t mask = wq->tasks_capacity - 1;
		struct __work_queue_task *task = &wq->tasks[thr->cursor & mask];
		task->func(task->arg);

		__sync_fetch_and_add(&wq->idle_count, 1);
	}

	return NULL;
}

void
ObscuraBusySpinWait()
{
	_mm_pause();
}

void
ObscuraYieldWait()
{
	sched_yield();
}

ObscuraWorkQueue *
ObscuraCreateWorkQueue(uint32_t threads_capacity, uint32_t tasks_capacity, PFN_ObscuraWaitStrategy wait_strategy,
	ObscuraAllocationCallbacks *allocator)
{
	assert((threads_capacity != 0) && ((threads_capacity & (threads_capacity - 1)) == 0));
	assert((tasks_capacity != 0) && ((tasks_capacity & (tasks_capacity - 1)) == 0));

	ObscuraWorkQueue *wq = allocator->allocation(sizeof(ObscuraWorkQueue), 8);

	wq->threads_capacity = threads_capacity;
	wq->threads = allocator->allocation(sizeof(struct __work_queue_thread) * threads_capacity, LEVEL1_DCACHE_LINESIZE);

	wq->tasks_capacity = tasks_capacity;
	wq->tasks = allocator->allocation(sizeof(struct __work_queue_task) * tasks_capacity, PAGESIZE);

	wq->wait_strategy = wait_strategy;

	wq->running = true;
	for (uint32_t i = 0; i < threads_capacity; i++) {
		if (pthread_create(&wq->threads[i].thread, NULL, &start_routine, wq)) {
			fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, strerror(errno));
                	exit(EXIT_FAILURE);
		}

		if (pthread_detach(wq->threads[i].thread)) {
			fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, strerror(errno));
                	exit(EXIT_FAILURE);
		}

		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(i, &cpuset);
		if (pthread_setaffinity_np(wq->threads[i].thread, sizeof(cpu_set_t), &cpuset)) {
			fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, strerror(errno));
                	exit(EXIT_FAILURE);
		}
			
	}

	wq->idle_count = threads_capacity;

	return wq;
}

void
ObscuraDestroyWorkQueue(ObscuraWorkQueue **ptr, ObscuraAllocationCallbacks *allocator)
{
	ObscuraWorkQueue *wq = *ptr;

	wq->running = false;
	for (uint32_t i = 0; i < wq->threads_capacity; i++) {
		if (pthread_cancel(wq->threads[i].thread)) {
			fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	allocator->free((*ptr)->tasks);
	allocator->free((*ptr)->threads);
	allocator->free(*ptr);

	*ptr = NULL;
}

void
ObscuraEnqueueTask(ObscuraWorkQueue *wq, PFN_ObscuraThreadFunction fn, void *arg)
{
	uint32_t i = wq->tasks_head_cursor;
	while (__builtin_expect(i >= wq->tasks_tail_cursor + wq->tasks_capacity, 0)) {
		if (!wq->running) {
			break;
		}

		uint32_t min = UINT_MAX;
		for (uint32_t j = 0; j < wq->threads_capacity; j++) {
			uint32_t cursor = wq->threads[j].cursor;

			asm volatile("" ::: "memory");

			if (cursor < min) {
				min = cursor;
			}
		}
		wq->tasks_tail_cursor = min;

		if (i < wq->tasks_tail_cursor + wq->tasks_capacity) {
			break;
		}

		wq->wait_strategy();
	}

	uint32_t mask = wq->tasks_capacity - 1;
	struct __work_queue_task *task = &wq->tasks[i & mask];
	task->func = fn;
	task->arg = arg;

	wq->tasks_head_cursor++;
}

void
ObscuraWaitAll(ObscuraWorkQueue *wq)
{
	while (wq->idle_count < wq->threads_capacity) {
		wq->wait_strategy();
	}
}
