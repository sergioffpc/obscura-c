#include "light.h"

ObscuraLight *ObscuraCreateLight(ObscuraLightSourceType type, ObscuraAllocationCallbacks *allocator) {
	ObscuraLight *light = allocator->allocation(sizeof(ObscuraLight), 8);
	light->type = type;

	switch (light->type) {
	case OBSCURA_LIGHT_SOURCE_TYPE_AMBIENT:
		light->source = allocator->allocation(sizeof(ObscuraLightAmbient), 8);
		break;
	case OBSCURA_LIGHT_SOURCE_TYPE_DIRECTIONAL:
		light->source = allocator->allocation(sizeof(ObscuraLightDirectional), 8);
		break;
	case OBSCURA_LIGHT_SOURCE_TYPE_POINT:
		light->source = allocator->allocation(sizeof(ObscuraLightPoint), 8);
		break;
	case OBSCURA_LIGHT_SOURCE_TYPE_SPOT:
		light->source = allocator->allocation(sizeof(ObscuraLightSpot), 8);
		break;
	}

	return light;
}

void ObscuraDestroyLight(ObscuraLight **ptr, ObscuraAllocationCallbacks *allocator) {
	allocator->free((*ptr)->source);
	allocator->free(*ptr);

	*ptr = NULL;
}
