#include <assert.h>
#include <float.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "camera.h"
#include "collision.h"
#include "material.h"
#include "renderer.h"
#include "scene.h"
#include "world.h"

struct ray_casts {
	struct {
		ObscuraNode		*node;
		ObscuraCollision	*collision;
	}		casts[256];
	uint32_t	casts_count;

	ObscuraRendererRay	*ray;
};

static void
enumerate_ray_casts(ObscuraNode *node, void *arg)
{
	if (ObscuraFindAnyComponent(node, OBSCURA_COMPONENT_FAMILY_GEOMETRY) != NULL) {
		struct ray_casts *ray_casts = arg;

		ObscuraRendererRay *ray = ray_casts->ray;

		ObscuraComponent *component = ObscuraFindAnyComponent(node, OBSCURA_COMPONENT_FAMILY_BOUNDING_VOLUME);
		ObscuraBoundingVolume *volume = component->component;

		ObscuraCollision *collision = ObscuraCreateCollision(&World.allocator);
		ObscuraCollidesWith(ray->volume, ray->position, volume, node->position, collision);
		if (collision->hit) {
			int i = ray_casts->casts_count++;

			ray_casts->casts[i].node = node;
			ray_casts->casts[i].collision = collision;
		} else {
			ObscuraDestroyCollision(&collision, &World.allocator);
		}
	}
}

static void
free_ray_casts(struct ray_casts *ray_casts)
{
	for (uint32_t i = 0; i < ray_casts->casts_count; i++) {
		ray_casts->casts[i].node = NULL;
		ObscuraDestroyCollision(&ray_casts->casts[i].collision, &World.allocator);
	}
	ray_casts->casts_count = 0;
}

static void
nearest_surface(struct ray_casts *ray_casts, int32_t *index)
{
	float znear = -FLT_MAX;

	for (uint32_t i = 0; i < ray_casts->casts_count; i++) {
		float zhit = ray_casts->casts[i].collision->hit_point[2];
		if (zhit > znear) {
			znear = zhit;

			*index = i;
		}
	}
}

static vec4
cast_color_filter(struct ray_casts *ray_casts, int32_t index)
{
	ObscuraNode *node = ray_casts->casts[index].node;

	ObscuraComponent *component = ObscuraFindAnyComponent(node, OBSCURA_COMPONENT_FAMILY_MATERIAL);
	ObscuraMaterial *material = component->component;

	vec4 rgba = { 0, 0, 1, 0 };
	switch (material->type) {
	case OBSCURA_MATERIAL_TYPE_COLOR:
		rgba = ((ObscuraMaterialColor *) material->material)->color;
	}

	return rgba;
}

static vec4
cast_depth_filter(struct ray_casts *ray_casts, int32_t index)
{
	ObscuraCollision *collision = ray_casts->casts[index].collision;
	float z = collision->hit_point[2];

	vec4 rgba = { z, z, z, 0 };

	return rgba;
}

static vec4
cast_normal_filter(struct ray_casts *ray_casts, int32_t index)
{
	ObscuraCollision *collision = ray_casts->casts[index].collision;

	vec4 rgba = {
		(collision->hit_normal[0] + 1) * 0.5,
		(collision->hit_normal[1] + 1) * 0.5,
		(collision->hit_normal[2] + 1) * 0.5,
		0,
	};

	return rgba;
}

ObscuraRendererRay *
ObscuraCreateRendererRay(ObscuraAllocationCallbacks *allocator)
{
	ObscuraRendererRay *ray = allocator->allocation(sizeof(ObscuraRendererRay), 8);
	ray->volume = ObscuraCreateBoundingVolume(allocator);

	ObscuraBindBoundingVolume(ray->volume, OBSCURA_BOUNDING_VOLUME_TYPE_RAY, allocator);

	return ray;
}

void
ObscuraDestroyRendererRay(ObscuraRendererRay **ptr, ObscuraAllocationCallbacks *allocator)
{
	ObscuraDestroyBoundingVolume(&(*ptr)->volume, allocator);
	allocator->free(*ptr);

	*ptr = NULL;
}

ObscuraRendererRay *
ObscuraBindRay(ObscuraRendererRay *ray, ObscuraRendererRayType type, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ray->type = type;

	return ray;
}

vec4
ObscuraCastRay(ObscuraRendererRay *ray)
{
	struct ray_casts ray_casts = {
		.casts       = {},
		.casts_count = 0,
		.ray         = ray,
	};
	ObscuraTraverseScene(World.scene, &enumerate_ray_casts, &ray_casts);

	int32_t index = INT32_MAX;
	nearest_surface(&ray_casts, &index);

	vec4 rgba = { 0, 0, 1, 0 };
	if (index != INT32_MAX) {
		ObscuraComponent *component = ObscuraFindAnyComponent(World.scene->view, OBSCURA_COMPONENT_FAMILY_CAMERA);
		ObscuraCamera *camera = component->component;

		switch (camera->filter) {
		case OBSCURA_CAMERA_FILTER_TYPE_COLOR:
			rgba = cast_color_filter(&ray_casts, index);
			break;
		case OBSCURA_CAMERA_FILTER_TYPE_DEPTH:
			rgba = cast_depth_filter(&ray_casts, index);
			break;
		case OBSCURA_CAMERA_FILTER_TYPE_NORMAL:
			rgba = cast_normal_filter(&ray_casts, index);
			break;
		default:
			assert(false);
			break;
		}
	}

	free_ray_casts(&ray_casts);

	return rgba;
}
