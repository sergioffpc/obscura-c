#ifndef __OBSCURA_RENDERER_H__
#define __OBSCURA_RENDERER_H__ 1

#include <stdint.h>

#include "collision.h"
#include "memory.h"
#include "stat.h"
#include "tensor.h"
#include "thread.h"
#include "world.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void	(*PFN_ObscuraPaintFunction)	(void *, int, int, uint32_t);

typedef struct ObscuraFramebuffer {
	int	width;
	int	height;

	void	*image;
	PFN_ObscuraPaintFunction	paint;
} ObscuraFramebuffer;

typedef enum ObscuraRendererRayType {
	OBSCURA_RENDERER_RAY_TYPE_CAMERA,
	OBSCURA_RENDERER_RAY_TYPE_REFLECTION,
	OBSCURA_RENDERER_RAY_TYPE_REFRACTION,
	OBSCURA_RENDERER_RAY_TYPE_SHADOW,
} ObscuraRendererRayType;

typedef struct ObscuraRendererRay {
	ObscuraRendererRayType	 type;
	vec4			 position;
	ObscuraBoundingVolume	*volume;
} ObscuraRendererRay;

extern ObscuraRendererRay *	ObscuraCreateRendererRay	(ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyRendererRay	(ObscuraRendererRay **, ObscuraAllocationCallbacks *);

extern ObscuraRendererRay *	ObscuraBindRay	(ObscuraRendererRay *, ObscuraRendererRayType, ObscuraAllocationCallbacks *);

typedef struct ObscuraRenderer {
	ObscuraCounters		counters;

	ObscuraAllocationCallbacks	*allocator;
	ObscuraExecutionCallbacks	*executor;

	ObscuraFramebuffer	framebuffer;

	ObscuraWorld		*world;

	uint32_t	  lights_capacity;
	uint32_t	  lights_count;
	ObscuraNode	**lights;
} ObscuraRenderer;

extern ObscuraRenderer *	ObscuraCreateRenderer	(ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyRenderer	(ObscuraRenderer **, ObscuraAllocationCallbacks *);

extern void ObscuraDraw	(ObscuraRenderer *)	__attribute__((hot));

#ifdef __cplusplus
}
#endif

#endif
