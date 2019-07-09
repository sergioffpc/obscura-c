#include <assert.h>
#include <stdbool.h>

#include "camera.h"

ObscuraCamera *
ObscuraCreateCamera(ObscuraAllocationCallbacks *allocator)
{
	ObscuraCamera *camera = allocator->allocation(sizeof(ObscuraCamera), 8);
	camera->filter = OBSCURA_CAMERA_FILTER_TYPE_COLOR;
	camera->anti_aliasing = OBSCURA_CAMERA_ANTI_ALIASING_TECHNIQUE_NONE;

	return camera;
}

void
ObscuraDestroyCamera(ObscuraCamera **ptr, ObscuraAllocationCallbacks *allocator)
{
	allocator->free((*ptr)->projection);
	allocator->free(*ptr);

	*ptr = NULL;
}

ObscuraCamera *
ObscuraBindProjection(ObscuraCamera *camera, ObscuraCameraProjectionType type, ObscuraAllocationCallbacks *allocator)
{
	camera->type = type;

	switch (camera->type) {
	case OBSCURA_CAMERA_PROJECTION_TYPE_PERSPECTIVE:
		camera->projection = allocator->allocation(sizeof(ObscuraCameraPerspective), 8);
		break;
	default:
		assert(false);
		break;
	}

	return camera;
}
