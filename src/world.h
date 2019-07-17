#ifndef __OBSCURA_WORLD_H__
#define __OBSCURA_WORLD_H__ 1

#include "memory.h"
#include "scene.h"
#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ObscuraWorld {
	ObscuraAllocationCallbacks	allocator;

	ObscuraWorkQueue	*work_queue;
	ObscuraScene		*scene;
} ObscuraWorld;

extern ObscuraWorld *	ObscuraCreateWorld	(void);
extern void		ObscuraDestroyWorld	(ObscuraWorld **);

extern void	ObscuraLoadWorld	(ObscuraWorld *, const char *);
extern void	ObscuraUnloadWorld	(ObscuraWorld *);

#ifdef __cplusplus
}
#endif

#endif
