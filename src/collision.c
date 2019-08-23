#include <assert.h>
#include <stdbool.h>

#include "collision.h"

static void
raysphereintersect(ObscuraBoundingVolumeRay *v1, vec4 p1, ObscuraBoundingVolumeSphere *v2, vec4 p2, ObscuraCollision *collision)
{
	vec4 d = p1 - p2;
	
	float a = vec4_dot(v1->direction, v1->direction);
	float b = 2 * vec4_dot(v1->direction, d);
	float c = vec4_dot(d, d) - (v2->radius * v2->radius);

	collision->hit = false;

	float x0, x1;
	if (quad_solver(a, b, c, &x0, &x1)) {
		float x = (x0 > x1 && x1 > 0) ? x1 : x0;
		if (x > 0) {
			collision->hit        = true;
			collision->hit_point  = p1 + v1->direction * x;
			collision->hit_normal = vec4_normalize(collision->hit_point - p2);
		}
	}
}

ObscuraBoundingVolume *
ObscuraCreateBoundingVolume(ObscuraAllocationCallbacks *allocator)
{
	ObscuraBoundingVolume *volume = allocator->allocation(sizeof(ObscuraBoundingVolume), 8);

	return volume;
}

void
ObscuraDestroyBoundingVolume(ObscuraBoundingVolume **ptr, ObscuraAllocationCallbacks *allocator)
{
	allocator->free((*ptr)->volume);
	allocator->free(*ptr);

	*ptr = NULL;
}

ObscuraBoundingVolume *
ObscuraBindBoundingVolume(ObscuraBoundingVolume *volume, ObscuraBoundingVolumeType type, ObscuraAllocationCallbacks *allocator)
{
	volume->type = type;

	switch (volume->type) {
	case OBSCURA_BOUNDING_VOLUME_TYPE_AABB:
		volume->volume = allocator->allocation(sizeof(ObscuraBoundingVolumeAABB), 8);
		break;
	case OBSCURA_BOUNDING_VOLUME_TYPE_RAY:
		volume->volume = allocator->allocation(sizeof(ObscuraBoundingVolumeRay), 8);
		break;
	case OBSCURA_BOUNDING_VOLUME_TYPE_SPHERE:
		volume->volume = allocator->allocation(sizeof(ObscuraBoundingVolumeSphere), 8);
		break;
	default:
		assert(false);
		break;
	}

	return volume;
}

ObscuraCollision *
ObscuraCreateCollision(ObscuraAllocationCallbacks *allocator)
{
	ObscuraCollision *collision = allocator->allocation(sizeof(ObscuraCollision), 8);

	return collision;
}

void
ObscuraDestroyCollision(ObscuraCollision **ptr, ObscuraAllocationCallbacks *allocator)
{
	allocator->free(*ptr);

	*ptr = NULL;
}

void
ObscuraCollidesWith(ObscuraBoundingVolume *v1, vec4 p1, ObscuraBoundingVolume *v2, vec4 p2, ObscuraCollision *collision)
{
	switch (v1->type) {
	case OBSCURA_BOUNDING_VOLUME_TYPE_AABB:
		switch (v2->type) {
		default:
			assert(false);
			break;
		}
		break;
	case OBSCURA_BOUNDING_VOLUME_TYPE_RAY:
		switch (v2->type) {
		case OBSCURA_BOUNDING_VOLUME_TYPE_SPHERE:
			raysphereintersect(v1->volume, p1, v2->volume, p2, collision);
			break;
		default:
			assert(false);
			break;
		}
		break;
	case OBSCURA_BOUNDING_VOLUME_TYPE_SPHERE:
		switch (v2->type) {
		case OBSCURA_BOUNDING_VOLUME_TYPE_RAY:
			raysphereintersect(v2->volume, p2, v1->volume, p1, collision);
			break;
		default:
			assert(false);
			break;
		}
		break;
	default:
		assert(false);
		break;
	}
}
