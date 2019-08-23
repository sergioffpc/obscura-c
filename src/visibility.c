#include <assert.h>

#include "visibility.h"

struct trace_ray_info {
	ObscuraVisible		*visible;
	ObscuraBoundingVolume	*ray;
	vec4			 position;
};

static void
trace(ObscuraNode *node, void *arg)
{
	if (ObscuraFindAnyComponent(node, OBSCURA_COMPONENT_FAMILY_GEOMETRY) != NULL) {
		struct trace_ray_info *info = arg;

		ObscuraComponent *component = ObscuraFindAnyComponent(node, OBSCURA_COMPONENT_FAMILY_BOUNDING_VOLUME);
		assert(component);
		ObscuraBoundingVolume *volume = component->component;

		ObscuraCollision collision = {};
		ObscuraCollidesWith(info->ray, info->position, volume, node->position, &collision);
		if (collision.hit) {
			if (!info->visible->geometry || collision.hit_point[2] > info->visible->collision.hit_point[2]) {
				info->visible->geometry = node;
				info->visible->collision = collision;
			}
		}
	}
}

ObscuraVisible
ObscuraTraceRay(ObscuraScene *scene, vec4 position, ObscuraBoundingVolume *ray)
{
	assert(ray->type == OBSCURA_BOUNDING_VOLUME_TYPE_RAY);

	ObscuraVisible visible = {};

	struct trace_ray_info info = {
		.visible  = &visible,
		.ray      = ray,
		.position = position,
	};
	ObscuraTraverseScene(scene, &trace, &info);

	return visible;
}
