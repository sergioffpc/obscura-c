#ifndef __OBSCURA_WORLD_H__
#define __OBSCURA_WORLD_H__ 1

#include "memory.h"
#include "scene.h"

typedef struct ObscuraWorld {
	ObscuraAllocationCallbacks	allocator;

	ObscuraScene	*scene;
} ObscuraWorld;

extern ObscuraWorld	World;

extern void	ObscuraLoadWorld	(const char *);
extern void	ObscuraUnloadWorld	(void);

#endif
