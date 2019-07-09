#ifndef __OBSCURA_MEMORY_H__
#define __OBSCURA_MEMORY_H__ 1

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void *	(*PFN_ObscuraAllocationFunction)	(size_t, size_t);
typedef void *	(*PFN_ObscuraReallocationFunction)	(void *, size_t, size_t);
typedef void	(*PFN_ObscuraFreeFunction)		(void *);

typedef struct ObscuraAllocationCallbacks {
	PFN_ObscuraAllocationFunction	allocation;
	PFN_ObscuraReallocationFunction	reallocation;
	PFN_ObscuraFreeFunction		free;
} ObscuraAllocationCallbacks;

#ifdef __cplusplus
}
#endif

#endif
