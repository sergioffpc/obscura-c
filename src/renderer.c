#include <assert.h>
#include <float.h>
#include <stdint.h>
#include <string.h>

#include "collision.h"
#include "material.h"
#include "renderer.h"
#include "scene.h"
#include "world.h"

struct collisions {
	struct {
		ObscuraSceneNode	*node;
		float			 zhit;
	}		objects[256];
	uint32_t	objects_count;

	ObscuraRendererRay	*ray;
};

static void find_collisions(ObscuraSceneNode *node, void *arg) {
	struct collisions *collisions = arg;

	ObscuraRendererRay *ray = collisions->ray;

	ObscuraSceneComponent *component = ObscuraFindComponent(node, OBSCURA_SCENE_COMPONENT_FAMILY_COLLIDABLE);
	ObscuraCollidable *collidable = component->component;

	ObscuraCollision collision;
	ObscuraCollidesWith(ray->collidable, ray->position, collidable, node->position, &collision);
	if (collision.hit) {
		int i = collisions->objects_count++;

		collisions->objects[i].node = node;
		collisions->objects[i].zhit = collision.hit_point[2];
	}
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

uint32_t ObscuraCastRay(ObscuraRendererRay *ray) {
	uint32_t color = 0xff;

	struct collisions collisions = {
		.objects       = {},
		.objects_count = 0,
		.ray           = ray,
	};
	ObscuraTraverseScene(World.scene, &find_collisions, &collisions);

	float znear = FLT_MAX;

	ObscuraSceneNode *node = NULL;
	for (uint32_t i = 0; i < collisions.objects_count; i++) {
		float zhit = collisions.objects[i].zhit;
		if (zhit < znear) {
			znear = zhit;
			node = collisions.objects[i].node;
		}
	}

	if (node != NULL) {
		ObscuraSceneComponent *component = ObscuraFindComponent(node, OBSCURA_SCENE_COMPONENT_FAMILY_MATERIAL);
		ObscuraMaterial *material = component->component;

		switch (material->type) {
		case OBSCURA_MATERIAL_TYPE_COLOR:
			color = ((ObscuraMaterialColor *) material->material)->color;
			break;
		default:
			assert(false);
			break;
		}
	}

	return color;
}
