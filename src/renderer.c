#include <assert.h>
#include <float.h>
#include <stdint.h>
#include <string.h>

#include "collision.h"
#include "material.h"
#include "renderer.h"
#include "scene.h"
#include "world.h"

struct ray_casts {
	struct {
		ObscuraSceneNode	*node;
		ObscuraCollision	*collision;
	}		casts[256];
	uint32_t	casts_count;

	ObscuraRendererRay	*ray;
};

static void enumerate_ray_casts(ObscuraSceneNode *node, void *arg) {
	if (ObscuraFindComponent(node, OBSCURA_SCENE_COMPONENT_FAMILY_GEOMETRY) != NULL) {
		struct ray_casts *ray_casts = arg;

		ObscuraRendererRay *ray = ray_casts->ray;

		ObscuraSceneComponent *component = ObscuraFindComponent(node, OBSCURA_SCENE_COMPONENT_FAMILY_COLLIDABLE);
		ObscuraCollidable *collidable = component->component;

		ObscuraCollision *collision = ObscuraCreateCollision(&World.allocator);
		ObscuraCollidesWith(ray->collidable, ray->position, collidable, node->position, collision);
		if (collision->hit) {
			int i = ray_casts->casts_count++;

			ray_casts->casts[i].node = node;
			ray_casts->casts[i].collision = collision;
		} else {
			ObscuraDestroyCollision(&collision, &World.allocator);
		}
	}
}

static void free_ray_casts(struct ray_casts *ray_casts) {
	for (uint32_t i = 0; i < ray_casts->casts_count; i++) {
		ray_casts->casts[i].node = NULL;
		ObscuraDestroyCollision(&ray_casts->casts[i].collision, &World.allocator);
	}
	ray_casts->casts_count = 0;
}

static void find_znear_ray_cast(struct ray_casts *ray_casts, int32_t *index) {
	float znear = -FLT_MAX;

	for (uint32_t i = 0; i < ray_casts->casts_count; i++) {
		float zhit = ray_casts->casts[i].collision->hit_point[2];
		if (zhit > znear) {
			znear = zhit;

			*index = i;
		}
	}
}

vec4 ray_cast_color(struct ray_casts *ray_casts, int32_t index) {
	ObscuraSceneNode *node = ray_casts->casts[index].node;

	ObscuraSceneComponent *component = ObscuraFindComponent(node, OBSCURA_SCENE_COMPONENT_FAMILY_MATERIAL);
	ObscuraMaterial *material = component->component;

	vec4 rgba = { 0, 0, 1, 0 };
	switch (material->type) {
	case OBSCURA_MATERIAL_TYPE_COLOR:
		rgba = ((ObscuraMaterialColor *) material->material)->color;
	}

	return rgba;
}

vec4 ray_cast_depth(struct ray_casts *ray_casts, int32_t index) {
	ObscuraCollision *collision = ray_casts->casts[index].collision;
	float z = collision->hit_point[2];

	vec4 rgba = { z, z, z, 0 };

	return rgba;
}

vec4 ray_cast_normal(struct ray_casts *ray_casts, int32_t index) {
	ObscuraCollision *collision = ray_casts->casts[index].collision;

	vec4 rgba = {
		(collision->hit_normal[0] + 1) * 0.5,
		(collision->hit_normal[1] + 1) * 0.5,
		(collision->hit_normal[2] + 1) * 0.5,
		0,
	};

	return rgba;
}

ObscuraRendererRay *ObscuraCreateRendererRay(ObscuraRendererRayType type, ObscuraAllocationCallbacks *allocator) {
	ObscuraRendererRay *ray = allocator->allocation(sizeof(ObscuraRendererRay), 8);
	ray->type = type;
	ray->collidable = ObscuraCreateCollidable(OBSCURA_COLLIDABLE_SHAPE_TYPE_RAY, allocator);

	return ray;
}

void ObscuraDestroyRendererRay(ObscuraRendererRay **ptr, ObscuraAllocationCallbacks *allocator) {
	ObscuraDestroyCollidable(&(*ptr)->collidable, allocator);
	allocator->free(*ptr);

	*ptr = NULL;
}

uint32_t ObscuraCastRay(ObscuraRenderer *renderer, ObscuraRendererRay *ray) {
	struct ray_casts ray_casts = {
		.casts       = {},
		.casts_count = 0,
		.ray         = ray,
	};
	ObscuraTraverseScene(World.scene, &enumerate_ray_casts, &ray_casts);

	int32_t index = INT32_MAX;
	find_znear_ray_cast(&ray_casts, &index);

	vec4 color = { 0.16, 0.16, 0.16 };
	if (index != INT32_MAX) {
		switch (renderer->buffer_type) {
		case OBSCURA_RENDERER_BUFFER_TYPE_COLOR:
			color = ray_cast_color(&ray_casts, index);
			break;
		case OBSCURA_RENDERER_BUFFER_TYPE_DEPTH:
			color = ray_cast_depth(&ray_casts, index);
			break;
		case OBSCURA_RENDERER_BUFFER_TYPE_NORMAL:
			color = ray_cast_normal(&ray_casts, index);
			break;
		}
	}

	free_ray_casts(&ray_casts);

	return OBSCURA_COLOR2UINT32(color);
}
