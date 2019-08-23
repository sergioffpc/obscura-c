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
	case OBSCURA_MATERIAL_EFFECT_TYPE_PHONG:
		material->effect = allocator->allocation(sizeof(ObscuraMaterialPhong), 8);
		break;
	default:
		assert(false);
		break;
	}

	return material;
}

ObscuraSurfaceAttributes
ObscuraSurfaceAttrs(ObscuraMaterial *material, vec4 coords)
{
    ObscuraSurfaceAttributes attrs = {};

	switch (material->type) {
	case OBSCURA_MATERIAL_EFFECT_TYPE_CONSTANT:
	{
		ObscuraMaterialConstant *effect = material->effect;
		switch (effect->emission.type) {
		case OBSCURA_MATERIAL_VALUE_TYPE_COLOR:
			attrs.emission_color = effect->emission.value.color;
			break;
		default:
			assert(false);
			break;
		}
	}
		break;
	case OBSCURA_MATERIAL_EFFECT_TYPE_PHONG:
	{
		ObscuraMaterialPhong *effect = material->effect;

		switch (effect->emission.type) {
		case OBSCURA_MATERIAL_VALUE_TYPE_COLOR:
			attrs.emission_color = effect->emission.value.color;
			break;
		default:
			assert(false);
			break;
		}

		switch (effect->ambient.type) {
		case OBSCURA_MATERIAL_VALUE_TYPE_COLOR:
			attrs.ambient_color = effect->ambient.value.color;
			break;
		default:
			assert(false);
			break;
		}

		switch (effect->diffuse.type) {
		case OBSCURA_MATERIAL_VALUE_TYPE_COLOR:
			attrs.diffuse_color = effect->diffuse.value.color;
			break;
		default:
			assert(false);
			break;
		}

		switch (effect->specular.type) {
		case OBSCURA_MATERIAL_VALUE_TYPE_COLOR:
			attrs.specular_color = effect->specular.value.color;
			break;
		default:
			assert(false);
			break;
		}

		switch (effect->shininess.type) {
		case OBSCURA_MATERIAL_VALUE_TYPE_COLOR:
			attrs.shininess = effect->shininess.value.color;
			break;
		default:
			assert(false);
			break;
		}
	}
		break;
	default:
		assert(false);
		break;
	}

	return attrs;
}
