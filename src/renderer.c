#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "camera.h"
#include "collision.h"
#include "light.h"
#include "material.h"
#include "renderer.h"
#include "world.h"

#define COLOR2UINT32(c) \
        (((uint32_t) (((int) ((c)[0] * 255) & 0xff) << 16) | (((int) ((c)[1] * 255) & 0xff) << 8) | ((int) ((c)[2] * 255) & 0xff)))

struct trace_info {
	ObscuraNode		*geometry;
	ObscuraCollision	 collision;

#define RAY_LIGHTS_CAPACITY	256
	ObscuraNode		*lights[RAY_LIGHTS_CAPACITY];
	uint32_t		 lights_count;

	ObscuraRendererRay	*ray;
};

static void
trace(ObscuraNode *node, void *arg)
{
	struct trace_info *info = arg;

	if (ObscuraFindAnyComponent(node, OBSCURA_COMPONENT_FAMILY_GEOMETRY) != NULL) {
		ObscuraRendererRay *ray = info->ray;

		ObscuraComponent *component = ObscuraFindAnyComponent(node, OBSCURA_COMPONENT_FAMILY_BOUNDING_VOLUME);
		ObscuraBoundingVolume *volume = component->component;

		ObscuraCollision collision = {};
		ObscuraCollidesWith(ray->volume, ray->position, volume, node->position, &collision);
		if (collision.hit) {
			if (!info->geometry || collision.hit_point[2] > info->collision.hit_point[2]) {
				info->geometry = node;
				info->collision = collision;
			}
		}
	}

	if (ObscuraFindAnyComponent(node, OBSCURA_COMPONENT_FAMILY_LIGHT) != NULL) {
		int i = info->lights_count++;
		assert(i < RAY_LIGHTS_CAPACITY);

		info->lights[i] = node;
	}
}

static vec4
shade(struct trace_info *info)
{
	ObscuraComponent *component = ObscuraFindAnyComponent(info->geometry, OBSCURA_COMPONENT_FAMILY_MATERIAL);
	ObscuraMaterial *material = component->component;

	vec4 color = { 0, 0, 1, 0 };
	switch (material->type) {
	case OBSCURA_MATERIAL_EFFECT_TYPE_CONSTANT:
	{
		vec4 ambient = { 0, 0, 0, 0 };
		for (uint32_t i = 0; i < info->lights_count; i++) {
			ObscuraComponent *component = ObscuraFindComponent(info->lights[i], OBSCURA_COMPONENT_FAMILY_LIGHT,
				OBSCURA_LIGHT_SOURCE_TYPE_AMBIENT);
			if (component != NULL) {
				ObscuraLightAmbient *light = ((ObscuraLight *) component->component)->source;
				ambient += light->color;
			}
		}

		const float al = 1;

		ObscuraMaterialConstant *effect = material->effect;
		switch (effect->emission.type) {
		case OBSCURA_MATERIAL_VALUE_TYPE_COLOR:
			color = effect->emission.value.color + ambient * al;
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

	return color;
}

static vec4
cast(ObscuraRenderer *renderer, ObscuraRendererRay *ray)
{
	__sync_fetch_and_add(&renderer->cast_count[ray->type], 1);

	ObscuraScene *scene = renderer->world->scene;

	struct trace_info info = {
		.ray = ray,
	};
	ObscuraTraverseScene(scene, &trace, &info);

	vec4 color = { 0, 0, 1, 0 };
	if (info.collision.hit) {
		ObscuraComponent *component = ObscuraFindAnyComponent(scene->view, OBSCURA_COMPONENT_FAMILY_CAMERA);
		ObscuraCamera *camera = component->component;

		switch (camera->filter) {
		case OBSCURA_CAMERA_FILTER_TYPE_COLOR:
			color = shade(&info);
			break;
		case OBSCURA_CAMERA_FILTER_TYPE_DEPTH:
			color[0] = color[1] = color[2] = info.collision.hit_point[2];
			break;
		case OBSCURA_CAMERA_FILTER_TYPE_NORMAL:
			color[0] = (info.collision.hit_normal[0] + 1) * 0.5;
			color[1] = (info.collision.hit_normal[1] + 1) * 0.5;
			color[2] = (info.collision.hit_normal[2] + 1) * 0.5;
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
	ObscuraFramebuffer *framebuffer = renderer->framebuffer;

	ObscuraNode *view = renderer->world->scene->view;
	ObscuraComponent *component = ObscuraFindComponent(view, OBSCURA_COMPONENT_FAMILY_CAMERA,
		OBSCURA_CAMERA_PROJECTION_TYPE_PERSPECTIVE);
	ObscuraCamera *camera = component->component;
	ObscuraCameraPerspective *projection = camera->projection;

	mat4 transformation = {};
	mat4_lookat(view->position, view->interest, view->up, transformation);

	ObscuraBoundingVolumeRay bounds = {
		.direction = {},
	};
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

					vec4 p = { pixel_camera_x, pixel_camera_y, -1, 1 };

					bounds.direction = mat4_transform(transformation, p);
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

					vec4 p = { pixel_camera_x, pixel_camera_y, -1, 1 };

					bounds.direction = mat4_transform(transformation, p);
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

void
ObscuraDraw(ObscuraRenderer *renderer)
{
	explicit_bzero(&renderer->cast_count, sizeof(renderer->cast_count));

	ObscuraWorkQueue *wq = renderer->world->work_queue;

	uint32_t partitions_count = wq->threads_capacity;
	struct partition_info partitions[partitions_count];

	size_t partition_size = renderer->framebuffer->height / partitions_count;
	for (uint32_t i = 0; i < partitions_count; i++) {
		partitions[i].y0 = i * partition_size;
		partitions[i].y1 = partitions[i].y0 + partition_size;
		partitions[i].renderer = renderer;

		ObscuraEnqueueTask(wq, &draw, &partitions[i]);
	}
	ObscuraWaitAll(wq);
}
