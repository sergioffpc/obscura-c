#include <assert.h>
#include <stdbool.h>

#include "material.h"

ObscuraMaterial *
ObscuraCreateMaterial(ObscuraAllocationCallbacks *allocator)
{
	ObscuraMaterial *material = allocator->allocation(sizeof(ObscuraMaterial), 8);

	return material;
}

void
ObscuraDestroyMaterial(ObscuraMaterial **ptr, ObscuraAllocationCallbacks *allocator)
{
	allocator->free((*ptr)->material);
	allocator->free(*ptr);

	*ptr = NULL;
}

ObscuraMaterial *
ObscuraBindMaterial(ObscuraMaterial *material, ObscuraMaterialType type, ObscuraAllocationCallbacks *allocator)
{
	material->type = type;

	switch (material->type) {
	case OBSCURA_MATERIAL_TYPE_COLOR:
		material->material = allocator->allocation(sizeof(ObscuraMaterialColor), 8);
		break;
	default:
		assert(false);
		break;
	}

	return material;
}
