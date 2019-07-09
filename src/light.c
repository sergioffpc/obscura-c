#include <assert.h>
#include <stdbool.h>

#include "light.h"

ObscuraLight *
ObscuraCreateLight(ObscuraAllocationCallbacks *allocator)
{
	ObscuraLight *light = allocator->allocation(sizeof(ObscuraLight), 8);

	return light;
}

void
ObscuraDestroyLight(ObscuraLight **ptr, ObscuraAllocationCallbacks *allocator)
{
	allocator->free((*ptr)->source);
	allocator->free(*ptr);

	*ptr = NULL;
}

ObscuraLight *
ObscuraBindSource(ObscuraLight *light, ObscuraLightSourceType type, ObscuraAllocationCallbacks *allocator)
{
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
	default:
		assert(false);
		break;
	}

	return light;
}
