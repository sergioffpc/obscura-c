#include <assert.h>
#include <stdbool.h>

#include "collision.h"

static void ray_ball_intersect(ObscuraCollidableRay *c1, vec4 p1, ObscuraCollidableBall *c2, vec4 p2, ObscuraCollision *collision) {
	vec4 d = p1 - p2;
	
	float a = vec4_dot(c1->direction, c1->direction);
	float b = 2 * vec4_dot(c1->direction, d);
	float c = vec4_dot(d, d) - (c2->radius * c2->radius);

	collision->hit = false;

	float x0;
	float x1;
	if (quad_solver(a, b, c, &x0, &x1)) {
		float x = (x0 > x1 && x1 > 0) ? x1 : x0;
		if (x > 0) {
			collision->hit        = true;
			collision->hit_point  = p1 + c1->direction * x;
			collision->hit_normal = vec4_normalize(collision->hit_point - p2);
		}
	}
}

ObscuraCollidable *ObscuraCreateCollidable(ObscuraCollidableShapeType type, ObscuraAllocationCallbacks *allocator) {
	ObscuraCollidable *collidable = allocator->allocation(sizeof(ObscuraCollidable), 8);
	collidable->type = type;

	switch (collidable->type) {
	case OBSCURA_COLLIDABLE_SHAPE_TYPE_BALL:
		collidable->shape = allocator->allocation(sizeof(ObscuraCollidableBall), 8);
		break;
	case OBSCURA_COLLIDABLE_SHAPE_TYPE_BOX:
		collidable->shape = allocator->allocation(sizeof(ObscuraCollidableBox), 8);
		break;
	case OBSCURA_COLLIDABLE_SHAPE_TYPE_FRUSTUM:
		collidable->shape = allocator->allocation(sizeof(ObscuraCollidableFrustum), 8);
		break;
	case OBSCURA_COLLIDABLE_SHAPE_TYPE_RAY:
		collidable->shape = allocator->allocation(sizeof(ObscuraCollidableRay), 8);
		break;
	default:
		assert(false);
		break;
	}

	return collidable;
}

void ObscuraDestroyCollidable(ObscuraCollidable **ptr, ObscuraAllocationCallbacks *allocator) {
	allocator->free((*ptr)->shape);
	allocator->free(*ptr);

	*ptr = NULL;
}

ObscuraCollision *ObscuraCreateCollision(ObscuraAllocationCallbacks *allocator) {
	ObscuraCollision *collision = allocator->allocation(sizeof(ObscuraCollision), 8);

	return collision;
}

void ObscuraDestroyCollision(ObscuraCollision **ptr, ObscuraAllocationCallbacks *allocator) {
	allocator->free(*ptr);

	*ptr = NULL;
}

void ObscuraCollidesWith(ObscuraCollidable *c1, vec4 p1, ObscuraCollidable *c2, vec4 p2, ObscuraCollision *collision) {
	switch (c1->type) {
	case OBSCURA_COLLIDABLE_SHAPE_TYPE_BALL:
		switch (c2->type) {
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_BALL:
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_BOX:
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_FRUSTUM:
			assert(false);
			break;
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_RAY:
			ray_ball_intersect(c2->shape, p2, c1->shape, p1, collision);
			break;
		default:
			assert(false);
			break;
		}
		break;
	case OBSCURA_COLLIDABLE_SHAPE_TYPE_BOX:
		switch (c2->type) {
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_BALL:
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_BOX:
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_FRUSTUM:
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_RAY:
			assert(false);
			break;
		default:
			assert(false);
			break;
		}
		break;
	case OBSCURA_COLLIDABLE_SHAPE_TYPE_FRUSTUM:
		switch (c2->type) {
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_BALL:
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_BOX:
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_FRUSTUM:
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_RAY:
			assert(false);
			break;
		default:
			assert(false);
			break;
		}
		break;
	case OBSCURA_COLLIDABLE_SHAPE_TYPE_RAY:
		switch (c2->type) {
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_BALL:
			ray_ball_intersect(c1->shape, p1, c2->shape, p2, collision);
			break;
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_BOX:
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_FRUSTUM:
		case OBSCURA_COLLIDABLE_SHAPE_TYPE_RAY:
			assert(false);
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
