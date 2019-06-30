#ifndef __OBSCURA_RENDERER_H__
#define __OBSCURA_RENDERER_H__ 1

#include <stdint.h>

#include "collision.h"
#include "mathematics.h"
#include "memory.h"

typedef enum ObscuraRendererRayType {
	OBSCURA_RENDERER_RAY_TYPE_CAMERA,
} ObscuraRendererRayType;

typedef struct ObscuraRendererRay {
	ObscuraRendererRayType	 type;
	vec4			 position;
	ObscuraCollidable	*collidable;
} ObscuraRendererRay;

extern ObscuraRendererRay *	ObscuraCreateRendererRay	(ObscuraRendererRayType, ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyRendererRay	(ObscuraRendererRay **, ObscuraAllocationCallbacks *);

extern uint32_t	ObscuraCastRay	(ObscuraRendererRay *)	__attribute__((hot));

#endif
