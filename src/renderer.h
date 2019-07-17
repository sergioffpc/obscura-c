#ifndef __OBSCURA_RENDERER_H__
#define __OBSCURA_RENDERER_H__ 1

#include <stdint.h>

#include "collision.h"
#include "memory.h"
#include "tensor.h"
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

	__RENDERER_RAY_TYPE_NUM_ELEMS,
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
	uint64_t	cast_count[__RENDERER_RAY_TYPE_NUM_ELEMS];

	ObscuraWorld		*world;
	ObscuraFramebuffer	*framebuffer;
} ObscuraRenderer;

extern void ObscuraDraw	(ObscuraRenderer *)	__attribute__((hot));

#ifdef __cplusplus
}
#endif

#endif
