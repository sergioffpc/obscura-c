#ifndef __OBSCURA_COLLISION_H__
#define __OBSCURA_COLLISION_H__ 1

#include <stdbool.h>

#include "memory.h"
#include "tensor.h"

typedef enum ObscuraCollidableShapeType {
	OBSCURA_COLLIDABLE_SHAPE_TYPE_BALL,
	OBSCURA_COLLIDABLE_SHAPE_TYPE_BOX,
	OBSCURA_COLLIDABLE_SHAPE_TYPE_FRUSTUM,
	OBSCURA_COLLIDABLE_SHAPE_TYPE_RAY,
} ObscuraCollidableShapeType;

typedef struct ObscuraCollidable {
	ObscuraCollidableShapeType	 type;
	void				*shape;
} ObscuraCollidable;

extern ObscuraCollidable *	ObscuraCreateCollidable		(ObscuraCollidableShapeType, ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyCollidable	(ObscuraCollidable **, ObscuraAllocationCallbacks *);

typedef struct ObscuraCollision {
	bool	hit;
	vec4	hit_point;
	vec4	hit_normal;
} ObscuraCollision;

extern ObscuraCollision *	ObscuraCreateCollision	(ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyCollision	(ObscuraCollision **, ObscuraAllocationCallbacks *);

extern void	ObscuraCollidesWith	(ObscuraCollidable *, vec4, ObscuraCollidable *, vec4, ObscuraCollision *);

typedef struct ObscuraCollidableBall {
	float	radius;
} ObscuraCollidableBall;

typedef struct ObscuraCollidableBox {
	float	width;
	float	height;
	float	depth;
} ObscuraCollidableBox;

typedef struct ObscuraCollidableFrustum {
	float	bottom;
	float	left;
	float	right;
	float	top;
	float	near;
	float	far;
} ObscuraCollidableFrustum;

typedef struct ObscuraCollidableRay {
	vec4	direction;
} ObscuraCollidableRay;

#endif
