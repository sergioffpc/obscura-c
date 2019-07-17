#ifndef __OBSCURA_MATERIAL_H__
#define __OBSCURA_MATERIAL_H__ 1

#include <stdint.h>

#include "memory.h"
#include "tensor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ObscuraMaterialEffectType {
	OBSCURA_MATERIAL_EFFECT_TYPE_CONSTANT,
	OBSCURA_MATERIAL_EFFECT_TYPE_PHONG,
} ObscuraMaterialEffectType;

typedef struct ObscuraMaterial {
	ObscuraMaterialEffectType	 type;
	void				*effect;
} ObscuraMaterial;

extern ObscuraMaterial *	ObscuraCreateMaterial(ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyMaterial(ObscuraMaterial **, ObscuraAllocationCallbacks *);

extern ObscuraMaterial *	ObscuraBindEffect(ObscuraMaterial *, ObscuraMaterialEffectType, ObscuraAllocationCallbacks *);

struct __material_color_or_texture {
	enum {
		OBSCURA_MATERIAL_VALUE_TYPE_COLOR,
		OBSCURA_MATERIAL_VALUE_TYPE_TEXTURE,
	}	type;
	union {
		vec4	color;
	}	value;
};

/*
 * Produces a constantly shaded surface that is independent of lighting.
 *
 * The reflected color is calculated as:
 *   color = emission + ambient * al
 * where:
 *   al - constant amount of ambient light contribution coming from the scene.
 */
typedef struct ObscuraMaterialConstant {
	struct __material_color_or_texture	emission;
	struct __material_color_or_texture	reflective;
	float			reflectivity;
	struct __material_color_or_texture	transparent;
	float			transparency;
	float			index_of_refraction;
} ObscuraMaterialConstant;

/*
 * Produces a shaded surface where the specular reflection is shaded according the Phong BRDF
 * approximation.
 *
 * The phong shader uses the common Phong shading equation, that is:
 *   color = emission + ambient * al + diffuse * max(dot(N, L), 0) + specular * max(dot(R, I, 0)^(shininess)
 * where:
 *   al - constant amount of ambient light contribution coming from the scene.
 *   N  - normal vector.
 *   L  - light vector.
 *   I  - eye vector.
 *   R  - perfect reflection vector.
 */
typedef struct ObscuraMaterialPhong {
	struct __material_color_or_texture	emission;
	struct __material_color_or_texture	ambient;
	struct __material_color_or_texture	diffuse;
	struct __material_color_or_texture	specular;
	struct __material_color_or_texture	shininess;
	struct __material_color_or_texture	reflective;
	float			reflectivity;
	struct __material_color_or_texture	transparent;
	float			transparency;
	float			index_of_refraction;
} ObscuraMaterialPhong;

#ifdef __cplusplus
}
#endif

#endif
