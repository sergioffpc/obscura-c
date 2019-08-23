#ifndef __OBSCURA_MATERIAL_H__
#define __OBSCURA_MATERIAL_H__ 1

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

extern ObscuraMaterial *	ObscuraCreateMaterial	(ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyMaterial	(ObscuraMaterial **, ObscuraAllocationCallbacks *);

extern ObscuraMaterial *	ObscuraBindEffect	(ObscuraMaterial *, ObscuraMaterialEffectType, ObscuraAllocationCallbacks *);

struct __material_color_or_texture {
	enum {
		OBSCURA_MATERIAL_VALUE_TYPE_COLOR,
		OBSCURA_MATERIAL_VALUE_TYPE_TEXTURE,
	}	type;
	union {
		vec4	color;
	}	value;
};

typedef struct ObscuraSurfaceAttributes {
	vec4	emission_color;
	vec4	ambient_color;
	vec4	diffuse_color;
	vec4	specular_color;
	vec4	shininess;
} ObscuraSurfaceAttributes;

extern ObscuraSurfaceAttributes	ObscuraSurfaceAttrs	(ObscuraMaterial *, vec4);

/*
 * Produces a constantly shaded surface that is independent of lighting.
 */
typedef struct ObscuraMaterialConstant {
	struct __material_color_or_texture	emission;
	struct __material_color_or_texture	reflective;
	float					reflectivity;
	struct __material_color_or_texture	transparent;
	float					transparency;
	float					index_of_refraction;
} ObscuraMaterialConstant;

/*
 * Produces a shaded surface where the specular reflection is shaded according the Phong BRDF
 * approximation.
 */
typedef struct ObscuraMaterialPhong {
	struct __material_color_or_texture	emission;
	struct __material_color_or_texture	ambient;
	struct __material_color_or_texture	diffuse;
	struct __material_color_or_texture	specular;
	struct __material_color_or_texture	shininess;
	struct __material_color_or_texture	reflective;
	float					reflectivity;
	struct __material_color_or_texture	transparent;
	float					transparency;
	float					index_of_refraction;
} ObscuraMaterialPhong;

#ifdef __cplusplus
}
#endif

#endif
