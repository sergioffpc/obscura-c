#ifndef __OBSCURA_WORLD_H__
#define __OBSCURA_WORLD_H__ 1

#include "memory.h"
#include "scene.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ObscuraWorld {
	ObscuraScene		*scene;
} ObscuraWorld;

extern ObscuraWorld *	ObscuraCreateWorld	(ObscuraAllocationCallbacks *);
extern void		ObscuraDestroyWorld	(ObscuraWorld **, ObscuraAllocationCallbacks *);

extern void	ObscuraLoadWorld	(ObscuraWorld *, const char *, ObscuraAllocationCallbacks *);
extern void	ObscuraUnloadWorld	(ObscuraWorld *, ObscuraAllocationCallbacks *);

#ifdef __cplusplus
}
#endif

#endif
