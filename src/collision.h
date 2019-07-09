#ifndef __OBSCURA_COLLISION_H__
#define __OBSCURA_COLLISION_H__ 1

#include <stdbool.h>

#include "memory.h"
#include "tensor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ObscuraBoundingVolumeType {
	OBSCURA_BOUNDING_VOLUME_TYPE_BALL,
	OBSCURA_BOUNDING_VOLUME_TYPE_BOX,
	OBSCURA_BOUNDING_VOLUME_TYPE_CONE,
	OBSCURA_BOUNDING_VOLUME_TYPE_FRUSTUM,
	OBSCURA_BOUNDING_VOLUME_TYPE_RAY,
} ObscuraBoundingVolumeType;

typedef struct ObscuraBoundingVolume {
	ObscuraBoundingVolumeType	 type;
	void				*volume;
} ObscuraBoundingVolume;

extern ObscuraBoundingVolume *	ObscuraCreateBoundingVolume	(ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyBoundingVolume	(ObscuraBoundingVolume **, ObscuraAllocationCallbacks *);

extern ObscuraBoundingVolume *	ObscuraBindBoundingVolume	(ObscuraBoundingVolume *, ObscuraBoundingVolumeType,
	ObscuraAllocationCallbacks *);

typedef struct ObscuraCollision {
	bool	hit;
	vec4	hit_point;
	vec4	hit_normal;
} ObscuraCollision;

extern ObscuraCollision *	ObscuraCreateCollision	(ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyCollision	(ObscuraCollision **, ObscuraAllocationCallbacks *);

extern void	ObscuraCollidesWith	(ObscuraBoundingVolume *, vec4, ObscuraBoundingVolume *, vec4, ObscuraCollision *);

typedef struct ObscuraBoundingVolumeBall {
	float	radius;
} ObscuraBoundingVolumeBall;

typedef struct ObscuraBoundingVolumeBox {
	vec4	half_extents;
} ObscuraBoundingVolumeBox;

typedef struct ObscuraBoundingVolumeCone {
	float	height;
	float	radius;
} ObscuraBoundingVolumeCone;

typedef struct ObscuraBoundingVolumeFrustum {
	float	bottom;
	float	left;
	float	right;
	float	top;
	float	near;
	float	far;
} ObscuraBoundingVolumeFrustum;

typedef struct ObscuraBoundingVolumeRay {
	vec4	direction;
} ObscuraBoundingVolumeRay;

#ifdef __cplusplus
}
#endif

#endif
