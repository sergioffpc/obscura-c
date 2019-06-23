#include "camera.h"

ObscuraCamera *ObscuraCreateCamera(ObscuraCameraProjectionType type, ObscuraAllocationCallbacks *allocator) {
	ObscuraCamera *camera = allocator->allocation(sizeof(ObscuraCamera), 8);
	camera->type = type;

	switch (camera->type) {
	case OBSCURA_CAMERA_PROJECTION_TYPE_PERSPECTIVE:
		camera->projection = allocator->allocation(sizeof(ObscuraCameraPerspective), 8);
		break;
	}

	return camera;
}

void ObscuraDestroyCamera(ObscuraCamera **ptr, ObscuraAllocationCallbacks *allocator) {
	allocator->free((*ptr)->projection);
	allocator->free(*ptr);

	*ptr = NULL;
}
