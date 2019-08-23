#ifndef __OBSCURA_VISIBILITY_H__
#define __OBSCURA_VISIBILITY_H__ 1

#include "collision.h"
#include "scene.h"
#include "tensor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ObscuraVisible {
	ObscuraNode		*geometry;
	ObscuraCollision	 collision;
} ObscuraVisible;

extern ObscuraVisible	ObscuraTraceRay	(ObscuraScene *, vec4, ObscuraBoundingVolume *);

#ifdef __cplusplus
}
#endif

#endif
