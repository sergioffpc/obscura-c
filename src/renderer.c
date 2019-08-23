#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "camera.h"
#include "light.h"
#include "material.h"
#include "renderer.h"
#include "shade.h"
#include "stat.h"
#include "visibility.h"

static bool
overcast(ObscuraRenderer *renderer, ObscuraLight *light, vec4 position, vec4 intersect)
{
	ObscuraScene *scene = renderer->world->scene;

	ObscuraBoundingVolumeRay bounds = {};
	ObscuraBoundingVolume volume = {
		.type   = OBSCURA_BOUNDING_VOLUME_TYPE_RAY,
		.volume = &bounds,
	};
	ObscuraRendererRay ray = {
		.type     = OBSCURA_RENDERER_RAY_TYPE_SHADOW,
		.position = intersect,
		.volume   = &volume,
	};

	ObscuraVisible occlusion = {};

    switch (light->type) {
    case OBSCURA_LIGHT_SOURCE_TYPE_AMBIENT:
        break;
    case OBSCURA_LIGHT_SOURCE_TYPE_DIRECTIONAL:
		__sync_fetch_and_add(&renderer->counters.counter[OBSCURA_COUNTER_TYPE_SHADOW], 1);

        bounds.direction = ((ObscuraLightDirectional *) light->source)->direction;

        occlusion = ObscuraTraceRay(scene, ray.position, ray.volume);
        break;
    case OBSCURA_LIGHT_SOURCE_TYPE_POINT:
		__sync_fetch_and_add(&renderer->counters.counter[OBSCURA_COUNTER_TYPE_SHADOW], 1);

        bounds.direction = position - intersect;
        bounds.direction = vec4_normalize(bounds.direction);

        occlusion = ObscuraTraceRay(scene, ray.position, ray.volume);
        break;
    case OBSCURA_LIGHT_SOURCE_TYPE_SPOT:
		__sync_fetch_and_add(&renderer->counters.counter[OBSCURA_COUNTER_TYPE_SHADOW], 1);

        occlusion = ObscuraTraceRay(scene, ray.position, ray.volume);
        break;
    default:
        assert(false);
        break;
    }

    return occlusion.collision.hit;
}

static vec4
shade(ObscuraRenderer *renderer, ObscuraVisible *visible)
{
	ObscuraScene *scene = renderer->world->scene;
	ObscuraNode *view = scene->view;

	vec4 color = { 0, 0, 0, 0 };
	for (uint32_t i = 0; i < renderer->lights_count; i++) {
		ObscuraNode *light = renderer->lights[i];

		ObscuraLight *l = ObscuraFindAnyComponent(light, OBSCURA_COMPONENT_FAMILY_LIGHT)->component;
		if (!overcast(renderer, l, light->position, visible->collision.hit_point)) {
			color = blend(color, ObscuraShade(visible, light, view));
		}
	}

	return color;
}

static vec4
cast(ObscuraRenderer *renderer, ObscuraRendererRay *ray)
{
	__sync_fetch_and_add(&renderer->counters.counter[OBSCURA_COUNTER_TYPE_CAMERA], 1);

	ObscuraScene *scene = renderer->world->scene;
	ObscuraVisible visible = ObscuraTraceRay(scene, ray->position, ray->volume);

	vec4 color = { 0, 0, 1, 0 };
	if (visible.collision.hit) {
		ObscuraComponent *component = ObscuraFindAnyComponent(scene->view, OBSCURA_COMPONENT_FAMILY_CAMERA);
		ObscuraCamera *camera = component->component;

		switch (camera->filter) {
		case OBSCURA_CAMERA_FILTER_TYPE_COLOR:
			color = shade(renderer, &visible);
			break;
		case OBSCURA_CAMERA_FILTER_TYPE_DEPTH:
			color[0] = color[1] = color[2] = visible.collision.hit_point[2];
			break;
		case OBSCURA_CAMERA_FILTER_TYPE_NORMAL:
			color[0] = (visible.collision.hit_normal[0] + 1) * 0.5;
			color[1] = (visible.collision.hit_normal[1] + 1) * 0.5;
			color[2] = (visible.collision.hit_normal[2] + 1) * 0.5;
			break;
		default:
			assert(false);
			break;
		}
	}

	return color;
}

struct partition_info {
	int	 y0, y1;

	ObscuraRenderer	*renderer;
};

static void *
draw(void *arg)
{
	struct partition_info *info = arg;

	ObscuraRenderer *renderer = info->renderer;
	ObscuraFramebuffer *framebuffer = &renderer->framebuffer;

	ObscuraNode *view = renderer->world->scene->view;
	ObscuraCamera *camera = ObscuraFindComponent(view, OBSCURA_COMPONENT_FAMILY_CAMERA,
		OBSCURA_CAMERA_PROJECTION_TYPE_PERSPECTIVE)->component;
	ObscuraCameraPerspective *projection = camera->projection;

	mat4 transformation = {};
	mat4_lookat(view->position, view->interest, view->up, transformation);

	ObscuraBoundingVolumeRay bounds = {};
	ObscuraBoundingVolume volume = {
		.type   = OBSCURA_BOUNDING_VOLUME_TYPE_RAY,
		.volume = &bounds,
	};
	ObscuraRendererRay ray = {
		.type     = OBSCURA_RENDERER_RAY_TYPE_CAMERA,
		.position = view->position,
		.volume   = &volume,
	};

	for (int y = info->y0; y < info->y1; y++) {
		for (int x = 0; x < framebuffer->width; x++) {
			vec4 color = { 0, 0, 0, 0 };
			switch (camera->anti_aliasing) {
			case OBSCURA_CAMERA_ANTI_ALIASING_TECHNIQUE_SSAA_STOCHASTIC:
				for (uint32_t i = 0; i < camera->samples_count; i++) {
					float pixel_ndc_x = (x + drand48()) / framebuffer->width;
					float pixel_ndc_y = (y + drand48()) / framebuffer->height;

					float pixel_screen_x = 2 * pixel_ndc_x - 1;
					float pixel_screen_y = 1 - 2 * pixel_ndc_y;

					float scale = tanf(DEG2RADF(projection->yfov / 2));
					float pixel_camera_x = pixel_screen_x * projection->aspect_ratio * scale;
					float pixel_camera_y = pixel_screen_y * scale;

					vec4 pt = { pixel_camera_x, pixel_camera_y, -1, 1 };

					bounds.direction = mat4_transform(transformation, pt);
					bounds.direction = vec4_normalize(bounds.direction);

					color += cast(renderer, &ray);
				}
				color /= (float) camera->samples_count;
				break;
			default:
				{
					float pixel_ndc_x = (x + 0.5) / framebuffer->width;
					float pixel_ndc_y = (y + 0.5) / framebuffer->height;

					float pixel_screen_x = 2 * pixel_ndc_x - 1;
					float pixel_screen_y = 1 - 2 * pixel_ndc_y;

					float scale = tanf(DEG2RADF(projection->yfov / 2));
					float pixel_camera_x = pixel_screen_x * projection->aspect_ratio * scale;
					float pixel_camera_y = pixel_screen_y * scale;

					vec4 pt = { pixel_camera_x, pixel_camera_y, -1, 1 };

					bounds.direction = mat4_transform(transformation, pt);
					bounds.direction = vec4_normalize(bounds.direction);

					color = cast(renderer, &ray);
				}
				break;
			}

			framebuffer->paint(framebuffer->image, x, y, COLOR2UINT32(color));
		}
	}

	return NULL;
}

static void
enumlights(ObscuraNode *node, void *arg)
{
	ObscuraRenderer *renderer = arg;

	if (ObscuraFindAnyComponent(node, OBSCURA_COMPONENT_FAMILY_LIGHT) != NULL) {
		uint32_t i = renderer->lights_count++;
		renderer->lights[i] = node;
	}
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

ObscuraRenderer *
ObscuraCreateRenderer(ObscuraAllocationCallbacks *allocator)
{
	ObscuraRenderer *renderer = allocator->allocation(sizeof(ObscuraRenderer), 8);
	renderer->lights_capacity = 64;
	renderer->lights = allocator->allocation(sizeof(ObscuraNode *) * renderer->lights_capacity, 8);

	return renderer;
}

void
ObscuraDestroyRenderer(ObscuraRenderer **ptr, ObscuraAllocationCallbacks *allocator)
{
	ObscuraRenderer *renderer = *ptr;
	allocator->free(renderer->lights);
	allocator->free(renderer);

	*ptr = NULL;
}

void
ObscuraDraw(ObscuraRenderer *renderer)
{
	explicit_bzero(&renderer->counters, sizeof(ObscuraCounters));

	renderer->lights_count = 0;
	ObscuraTraverseScene(renderer->world->scene, &enumlights, renderer);

	uint32_t partitions_count = renderer->executor->nprocs();
	struct partition_info partitions[partitions_count];

	ObscuraFramebuffer *framebuffer = &renderer->framebuffer;
	size_t partition_size = framebuffer->height / partitions_count;

	for (uint32_t i = 0; i < partitions_count; i++) {
		partitions[i].y0 = i * partition_size;
		partitions[i].y1 = partitions[i].y0 + partition_size;
		partitions[i].renderer = renderer;

		renderer->executor->submit(&draw, &partitions[i]);
	}

	renderer->executor->wait();
}
