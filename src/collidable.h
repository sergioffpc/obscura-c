#ifndef __OBSCURA_COLLIDABLE_H__
#define __OBSCURA_COLLIDABLE_H__ 1

#include "memory.h"
#include "transform.h"

typedef enum ObscuraCollidableShapeType {
	OBSCURA_COLLIDABLE_SHAPE_TYPE_BOX,
	OBSCURA_COLLIDABLE_SHAPE_TYPE_FRUSTUM,
	OBSCURA_COLLIDABLE_SHAPE_TYPE_RAY,
	OBSCURA_COLLIDABLE_SHAPE_TYPE_SPHERE,
} ObscuraCollidableShapeType;

typedef struct ObscuraCollidable {
	ObscuraCollidableShapeType	 type;
	void				*shape;
} ObscuraCollidable;

extern ObscuraCollidable *	ObscuraCreateCollidable		(ObscuraCollidableShapeType, ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyCollidable	(ObscuraCollidable **, ObscuraAllocationCallbacks *);

typedef struct ObscuraCollidableRay {
	vec4	position;
	vec4	direction;
} ObscuraCollidableRay;

typedef struct ObscuraCollidableFrustum {
	float	znear;
	float	zfar;
} ObscuraCollidableFrustum;

#endif
