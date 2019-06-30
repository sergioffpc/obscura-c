#include "material.h"

ObscuraMaterial *ObscuraCreateMaterial(ObscuraMaterialType type, ObscuraAllocationCallbacks *allocator) {
	ObscuraMaterial *material = allocator->allocation(sizeof(ObscuraMaterial), 8);
	material->type = type;

	switch (material->type) {
	case OBSCURA_MATERIAL_TYPE_COLOR:
		material->material = allocator->allocation(sizeof(ObscuraMaterialColor), 8);
		break;
	}

	return material;
}

void ObscuraDestroyMaterial(ObscuraMaterial **ptr, ObscuraAllocationCallbacks *allocator) {
	allocator->free((*ptr)->material);
	allocator->free(*ptr);

	*ptr = NULL;
}
