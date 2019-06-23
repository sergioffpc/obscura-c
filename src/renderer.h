#ifndef __OBSCURA_RENDERER_H__
#define __OBSCURA_RENDERER_H__ 1

#include <stdint.h>

#include "collidable.h"

typedef struct ObscuraRendererRay {
	enum {
		OBSCURA_RENDERER_RAY_TYPE_CAMERA,
	}	type;

	ObscuraCollidableRay	*collidable;
} ObscuraRendererRay;

extern uint32_t	ObscuraCastRay	(ObscuraRendererRay *)	__attribute__((hot));

#endif
