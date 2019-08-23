#include <assert.h>
#include <stdbool.h>

#include "light.h"
#include "material.h"
#include "shade.h"

static vec4
enlighten(ObscuraSurfaceAttributes surface, vec4 normal, vec4 intersect, ObscuraLight *light, vec4 position, vec4 eye)
{
	vec4 color = {};

	switch (light->type) {
	case OBSCURA_LIGHT_SOURCE_TYPE_AMBIENT:
		color = blend(surface.emission_color + surface.ambient_color, ((ObscuraLightAmbient *) light->source)->color);
		break;
	case OBSCURA_LIGHT_SOURCE_TYPE_DIRECTIONAL:
	{
		ObscuraLightDirectional *directional = light->source;

		surface.diffuse_color *= clampf(vec4_dot(normal, directional->direction), 0, 1);

		vec4 reflection = vec4_reflect(directional->direction, normal);
		surface.specular_color *= clampf(vec4_dot(reflection, eye), 0, 1);
		surface.specular_color = vec4_pow(surface.specular_color, 1 - surface.shininess);

		color = blend(surface.emission_color + surface.ambient_color + surface.diffuse_color + surface.specular_color, directional->color);
	}
		break;
	case OBSCURA_LIGHT_SOURCE_TYPE_POINT:
	{
		ObscuraLightPoint *point = (ObscuraLightPoint *) light->source;

		vec4 direction = vec4_normalize(position - intersect);
		surface.diffuse_color *= clampf(vec4_dot(normal, direction), 0, 1);

		vec4 reflection = vec4_reflect(direction, normal);
		surface.specular_color *= clampf(vec4_dot(reflection, eye), 0, 1);
		surface.specular_color = vec4_pow(surface.specular_color, 1 - surface.shininess);

		float attenuation = OBSCURA_LIGHT_ATTENUATION(point, vec4_distance(intersect, position));
		color = blend(surface.emission_color + surface.ambient_color + surface.diffuse_color + surface.specular_color, point->color / attenuation);
	}
		break;
	case OBSCURA_LIGHT_SOURCE_TYPE_SPOT:
	{
		ObscuraLightSpot *spot = ((ObscuraLightSpot *) light->source);

		surface.diffuse_color *= clampf(vec4_dot(normal, spot->direction), 0, 1);

		vec4 reflection = vec4_reflect(spot->direction, normal);
		surface.specular_color *= clampf(vec4_dot(reflection, eye), 0, 1);
		surface.specular_color = vec4_pow(surface.specular_color, 1 - surface.shininess);

		float attenuation = OBSCURA_LIGHT_ATTENUATION(spot, vec4_distance(intersect, position));
		color = blend(surface.emission_color + surface.ambient_color + surface.diffuse_color + surface.specular_color, spot->color / attenuation);
	}
		break;
	default:
		assert(false);
		break;
	}

	return color;
}

vec4
ObscuraShade(ObscuraVisible *visible, ObscuraNode *light, ObscuraNode *view)
{
	vec4 coords = {};
	ObscuraSurfaceAttributes surface = {};

	ObscuraMaterial *m = ObscuraFindAnyComponent(visible->geometry, OBSCURA_COMPONENT_FAMILY_MATERIAL)->component;
	surface = ObscuraSurfaceAttrs(m, coords);

	vec4 color = {};
	
	ObscuraLight *l = ObscuraFindAnyComponent(light, OBSCURA_COMPONENT_FAMILY_LIGHT)->component;
	color = enlighten(surface, visible->collision.hit_normal, visible->collision.hit_point, l, light->position, view->interest);

	return color;
}
