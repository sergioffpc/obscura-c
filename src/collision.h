#ifndef __OBSCURA_COLLISION_H__
#define __OBSCURA_COLLISION_H__ 1

#include <stdbool.h>

#include "memory.h"
#include "tensor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ObscuraBoundingVolumeType {
	OBSCURA_BOUNDING_VOLUME_TYPE_AABB,
	OBSCURA_BOUNDING_VOLUME_TYPE_RAY,
	OBSCURA_BOUNDING_VOLUME_TYPE_SPHERE,
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

typedef struct ObscuraBoundingVolumeAABB {
	vec4	half_extents;
} ObscuraBoundingVolumeAABB;

typedef struct ObscuraBoundingVolumeRay {
	vec4	direction;
} ObscuraBoundingVolumeRay;

typedef struct ObscuraBoundingVolumeSphere {
	float	radius;
} ObscuraBoundingVolumeSphere;

#ifdef __cplusplus
}
#endif

#endif
