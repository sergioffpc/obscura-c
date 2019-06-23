#include "collidable.h"

ObscuraCollidable *ObscuraCreateCollidable(ObscuraCollidableShapeType type, ObscuraAllocationCallbacks *allocator) {
	ObscuraCollidable *collidable = allocator->allocation(sizeof(ObscuraCollidable), 8);
	collidable->type = type;

	switch (collidable->type) {
	case OBSCURA_COLLIDABLE_SHAPE_TYPE_FRUSTUM:
		collidable->shape = allocator->allocation(sizeof(ObscuraCollidableRay), 8);
		break;
	case OBSCURA_COLLIDABLE_SHAPE_TYPE_RAY:
		collidable->shape = allocator->allocation(sizeof(ObscuraCollidableRay), 8);
		break;
	}

	return collidable;
}

void ObscuraDestroyCollidable(ObscuraCollidable **ptr, ObscuraAllocationCallbacks *allocator) {
	allocator->free((*ptr)->shape);
	allocator->free(*ptr);

	*ptr = NULL;
}
