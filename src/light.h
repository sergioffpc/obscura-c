#ifndef __OBSCURA_LIGHT_H__
#define __OBSCURA_LIGHT_H__ 1

#include "tensor.h"

typedef enum ObscuraLightType {
	OBSCURA_LIGHT_TYPE_AMBIENT,
	OBSCURA_LIGHT_TYPE_DIRECTIONAL,
	OBSCURA_LIGHT_TYPE_POINT,
	OBSCURA_LIGHT_TYPE_SPOT,
} ObscuraLightType;

typedef struct ObscuraLight {
	ObscuraLightType	 type;
	void			*light;
} ObscuraLight;

/*
 * The ambient element declares the parameters required to describe an ambient light source. An
 * ambient light is one that lights everything evenly, regardless of location or orientation.
 */
typedef struct ObscuraLightAmbient {
	vec4	color;
} ObscuraLightAmbient;

/*
 * The directional element declares the parameters required to describe a directional light source. A
 * directional light is one that lights everything from the same direction, regardless of location.
 */
typedef struct ObscuraLightDirectional {
	vec4	color;
	vec4	direction;
} ObscuraLightDirectional;

/*
 * A point light source radiates light in all directions from a known location in space. The intensity
 * of a point light source is attenuated as the distance to the light source increases.
 */
typedef struct ObscuraLightPoint {
	vec4	color;

	float	constant_attenuation;
	float	linear_attenuation;
	float	quadratic_attenuation;
} ObscuraLightPoint;

/*
 * A spot light source radiates light in one direction in a cone shape from a known location in space. The
 * intensity of the light is attenuated as the radiation angle increases away from the direction of the light
 * source. The intensity of a spot light source is also attenuated as the distance to the light source increases.
 */
typedef struct ObscuraLightSpot {
	vec4	color;
	vec4	direction;

	float	constant_attenuation;
	float	linear_attenuation;
	float	quadratic_attenuation;

	float	falloff_angle;
	float	falloff_exponent;
} ObscuraLightSpot;

#define OBSCURA_LIGHT_ATTENUATION(l, d)	((l)->constant_attenuation + ((d) * (l)->linear_attenuation) + (((d) * (d)) * (l)->quadratic_attenuation))

#endif
