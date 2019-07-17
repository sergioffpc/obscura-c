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
	allocator->free((*ptr)->effect);
	allocator->free(*ptr);

	*ptr = NULL;
}

ObscuraMaterial *
ObscuraBindEffect(ObscuraMaterial *material, ObscuraMaterialEffectType type, ObscuraAllocationCallbacks *allocator)
{
	material->type = type;

	switch (material->type) {
	case OBSCURA_MATERIAL_EFFECT_TYPE_CONSTANT:
		material->effect = allocator->allocation(sizeof(ObscuraMaterialConstant), 8);
		break;
	default:
		assert(false);
		break;
	}

	return material;
}
