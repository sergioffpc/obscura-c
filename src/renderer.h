#ifndef __OBSCURA_RENDERER_H__
#define __OBSCURA_RENDERER_H__ 1

#include <stdint.h>

#include "collision.h"
#include "memory.h"
#include "tensor.h"

typedef enum ObscuraRendererBufferType {
	OBSCURA_RENDERER_BUFFER_TYPE_COLOR,
	OBSCURA_RENDERER_BUFFER_TYPE_DEPTH,
	OBSCURA_RENDERER_BUFFER_TYPE_NORMAL,
} ObscuraRendererBufferType;

typedef struct ObscuraRenderer {
	ObscuraRendererBufferType	buffer_type;
} ObscuraRenderer;

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

extern uint32_t	ObscuraCastRay	(ObscuraRenderer *, ObscuraRendererRay *)	__attribute__((hot));

#endif
