#ifndef __OBSCURA_RENDERER_H__
#define __OBSCURA_RENDERER_H__ 1

#include <stdint.h>

#include "collision.h"
#include "memory.h"
#include "scene.h"
#include "tensor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ObscuraRendererRayType {
	OBSCURA_RENDERER_RAY_TYPE_CAMERA,
} ObscuraRendererRayType;

typedef struct ObscuraRendererRay {
	ObscuraRendererRayType	 type;
	vec4			 position;
	ObscuraBoundingVolume	*volume;
} ObscuraRendererRay;

extern ObscuraRendererRay *	ObscuraCreateRendererRay	(ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyRendererRay	(ObscuraRendererRay **, ObscuraAllocationCallbacks *);

extern ObscuraRendererRay *	ObscuraBindRay	(ObscuraRendererRay *, ObscuraRendererRayType, ObscuraAllocationCallbacks *);

extern vec4	ObscuraCastRay	(ObscuraScene *, ObscuraRendererRay *, ObscuraAllocationCallbacks *)	__attribute__((hot));

#ifdef __cplusplus
}
#endif

#endif
