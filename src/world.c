#include <sys/sysinfo.h>

#include <assert.h>
#include <errno.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yaml.h>

#include "camera.h"
#include "collision.h"
#include "geometry.h"
#include "light.h"
#include "material.h"
#include "memory.h"
#include "scene.h"
#include "thread.h"
#include "world.h"

static void
mem_free(void *ptr)
{
	free(ptr);
}

static void *
mem_alloc(size_t size, size_t alignment)
{
	void *ptr = NULL;

	switch (posix_memalign(&ptr, alignment, size)) {
	case EINVAL:
		fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, "illegal alignment");
		exit(EXIT_FAILURE);
		break;
	case ENOMEM:
		fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, "out of memory");
		exit(EXIT_FAILURE);
		break;
	}

	explicit_bzero(ptr, size);

	return ptr;
}

static void *
mem_realloc(void *original, size_t size, size_t alignment)
{
	void *ptr = NULL;

	size_t usable_size = malloc_usable_size(original);
	if (usable_size == 0) {
		ptr = mem_alloc(size, alignment);
	} else if (usable_size > size) {
		ptr = mem_alloc(size, alignment);
		memcpy(ptr, original, usable_size);
		mem_free(original);
	} else {
		ptr = realloc(original, size);
		if (ptr == NULL) {
			fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}

	return ptr;
}

#define PARSER_STATE_CAPACITY	256
static struct parser_state {
	enum {
		PARSER_STATE_TYPE_CAMERA,
		PARSER_STATE_TYPE_CAMERA_ANTI_ALIASING,
		PARSER_STATE_TYPE_CAMERA_PERSPECTIVE,
		PARSER_STATE_TYPE_CAMERAS,
		PARSER_STATE_TYPE_BOUNDING_VOLUME,
		PARSER_STATE_TYPE_BOUNDING_VOLUME_BALL,
		PARSER_STATE_TYPE_BOUNDING_VOLUME_BOX,
		PARSER_STATE_TYPE_BOUNDING_VOLUMES,
		PARSER_STATE_TYPE_COLOR,
		PARSER_STATE_TYPE_COLOR_OR_TEXTURE,
		PARSER_STATE_TYPE_COMPONENTS,
		PARSER_STATE_TYPE_FLOAT,
		PARSER_STATE_TYPE_INT,
		PARSER_STATE_TYPE_GEOMETRY,
		PARSER_STATE_TYPE_GEOMETRY_SPHERE,
		PARSER_STATE_TYPE_GEOMETRIES,
		PARSER_STATE_TYPE_LIGHT,
		PARSER_STATE_TYPE_LIGHT_AMBIENT,
		PARSER_STATE_TYPE_LIGHT_DIRECTIONAL,
		PARSER_STATE_TYPE_LIGHT_POINT,
		PARSER_STATE_TYPE_LIGHT_SPOT,
		PARSER_STATE_TYPE_LIGHTS,
		PARSER_STATE_TYPE_MATERIAL,
		PARSER_STATE_TYPE_MATERIAL_CONSTANT,
		PARSER_STATE_TYPE_MATERIAL_PHONG,
		PARSER_STATE_TYPE_MATERIALS,
		PARSER_STATE_TYPE_REF,
		PARSER_STATE_TYPE_NODE,
		PARSER_STATE_TYPE_NODES,
		PARSER_STATE_TYPE_VECTOR4,
		PARSER_STATE_TYPE_SCENE,
	}	 type;
	void	*ptr;
}		evstack[PARSER_STATE_CAPACITY] = {};
static int8_t	evpointer = -1;

#define PARSER_ANCHOR_CAPACITY		256
#define PARSER_ANCHOR_NAME_CAPACITY	256
static struct parser_anchor {
	char	name[PARSER_ANCHOR_NAME_CAPACITY];
	void	*ptr;
}		anchors[PARSER_ANCHOR_CAPACITY] = {};
static int8_t	anchoridx = -1;

static void
camera_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator)
{
	ObscuraCamera *camera = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "perspective")) {
		ObscuraBindProjection(camera, OBSCURA_CAMERA_PROJECTION_TYPE_PERSPECTIVE, allocator);

		evstack[evpointer].type = PARSER_STATE_TYPE_CAMERA_PERSPECTIVE;
		evstack[evpointer].ptr  = camera;
	} else if (!strcmp((char *) event->data.scalar.value, "anti_aliasing")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_CAMERA_ANTI_ALIASING;
		evstack[evpointer].ptr  = camera;
	} else {
		assert(false);
	}
}

static void
camera_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
camera_anti_aliasing_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ObscuraCamera *camera = evstack[evpointer].ptr;

	evpointer++;
	evstack[evpointer].type = PARSER_STATE_TYPE_INT;

	if (!strcmp((char *) event->data.scalar.value, "technique")) {
		evstack[evpointer].ptr = &camera->anti_aliasing;
	} else if (!strcmp((char *) event->data.scalar.value, "samples_count")) {
		evstack[evpointer].ptr = &camera->samples_count;
	} else {
		assert(false);
	}
}

static void
camera_anti_aliasing_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
camera_perspective_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ObscuraCamera *camera = evstack[evpointer].ptr;

	evpointer++;
	evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;

	if (!strcmp((char *) event->data.scalar.value, "aspect_ratio")) {
		evstack[evpointer].ptr = &((ObscuraCameraPerspective *) camera->projection)->aspect_ratio;
	} else if (!strcmp((char *) event->data.scalar.value, "yfov")) {
		evstack[evpointer].ptr = &((ObscuraCameraPerspective *) camera->projection)->yfov;
	} else if (!strcmp((char *) event->data.scalar.value, "znear")) {
		evstack[evpointer].ptr = &((ObscuraCameraPerspective *) camera->projection)->znear;
	} else if (!strcmp((char *) event->data.scalar.value, "zfar")) {
		evstack[evpointer].ptr = &((ObscuraCameraPerspective *) camera->projection)->zfar;
	} else {
		assert(false);
	}
}

static void
camera_perspective_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
cameras_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator)
{
	ObscuraScene *scene = evstack[evpointer].ptr;

	evpointer++;
	ObscuraComponent *camera = ObscuraAcquireComponent(scene, OBSCURA_COMPONENT_FAMILY_CAMERA, allocator);
	assert(camera);

	evstack[evpointer].type = PARSER_STATE_TYPE_CAMERA;
	evstack[evpointer].ptr  = camera->component;

	if (event->data.scalar.anchor != NULL) {
		anchoridx++;
		strcpy(anchors[anchoridx].name, (char *) event->data.scalar.anchor);
		anchors[anchoridx].ptr = camera;
	} else {
		assert(false);
	}
}

static void
cameras_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
bounding_volume_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator)
{
	ObscuraBoundingVolume *volume = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "ball")) {
		ObscuraBindBoundingVolume(volume, OBSCURA_BOUNDING_VOLUME_TYPE_BALL, allocator);

		evstack[evpointer].type = PARSER_STATE_TYPE_BOUNDING_VOLUME_BALL;
		evstack[evpointer].ptr  = volume;
	} else if (!strcmp((char *) event->data.scalar.value, "box")) {
		ObscuraBindBoundingVolume(volume, OBSCURA_BOUNDING_VOLUME_TYPE_BOX, allocator);

		evstack[evpointer].type = PARSER_STATE_TYPE_BOUNDING_VOLUME_BOX;
		evstack[evpointer].ptr  = volume;
	} else {
		assert(false);
	}
}

static void
bounding_volume_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
bounding_volume_ball_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ObscuraBoundingVolume *volume = evstack[evpointer].ptr;

	evpointer++;
	evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;

	if (!strcmp((char *) event->data.scalar.value, "radius")) {
		evstack[evpointer].ptr = &((ObscuraBoundingVolumeBall *) volume->volume)->radius;
	} else {
		assert(false);
	}
}

static void
bounding_volume_ball_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
bounding_volume_box_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ObscuraBoundingVolume *volume = evstack[evpointer].ptr;

	evpointer++;
	evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR4;

	if (!strcmp((char *) event->data.scalar.value, "half_extents")) {
		evstack[evpointer].ptr = &((ObscuraBoundingVolumeBox *) volume->volume)->half_extents;
	} else {
		assert(false);
	}
}

static void
bounding_volume_box_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
bounding_volumes_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator)
{
	ObscuraScene *scene = evstack[evpointer].ptr;

	evpointer++;
	ObscuraComponent *volume = ObscuraAcquireComponent(scene, OBSCURA_COMPONENT_FAMILY_BOUNDING_VOLUME, allocator);
	assert(volume);

	evstack[evpointer].type = PARSER_STATE_TYPE_BOUNDING_VOLUME;
	evstack[evpointer].ptr  = volume->component;

	if (event->data.scalar.anchor != NULL) {
		anchoridx++;
		strcpy(anchors[anchoridx].name, (char *) event->data.scalar.anchor);
		anchors[anchoridx].ptr = volume;
	} else {
		assert(false);
	}
}

static void
bounding_volumes_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}


static void
components_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ObscuraNode *node = evstack[evpointer].ptr;

	for (int i = 0; i < anchoridx + 1; i++) {
		if (!strcmp(anchors[i].name, (char *) event->data.alias.anchor)) {
			ObscuraAttachComponent(node, anchors[i].ptr);
			break;
		}
	}
}

static void
components_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
geometry_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator)
{
	ObscuraGeometry *geometry = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "sphere")) {
		ObscuraBindGeometry(geometry, OBSCURA_GEOMETRY_TYPE_PARAMETRIC_SPHERE, allocator);

		evstack[evpointer].type = PARSER_STATE_TYPE_GEOMETRY_SPHERE;
		evstack[evpointer].ptr  = geometry;
	} else {
		assert(false);
	}
}

static void
geometry_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
geometry_sphere_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ObscuraGeometry *geometry = evstack[evpointer].ptr;

	evpointer++;
	evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;

	if (!strcmp((char *) event->data.scalar.value, "radius")) {
		evstack[evpointer].ptr = &((ObscuraGeometrySphere *) geometry->geometry)->radius;
	} else {
		assert(false);
	}
}

static void
geometry_sphere_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
geometries_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator)
{
	ObscuraScene *scene = evstack[evpointer].ptr;

	evpointer++;
	ObscuraComponent *geometry = ObscuraAcquireComponent(scene, OBSCURA_COMPONENT_FAMILY_GEOMETRY, allocator);
	assert(geometry);

	evstack[evpointer].type = PARSER_STATE_TYPE_GEOMETRY;
	evstack[evpointer].ptr  = geometry->component;

	if (event->data.scalar.anchor != NULL) {
		anchoridx++;
		strcpy(anchors[anchoridx].name, (char *) event->data.scalar.anchor);
		anchors[anchoridx].ptr = geometry;
	} else {
		assert(false);
	}
}

static void
geometries_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
light_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator)
{
	ObscuraLight *light = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "ambient")) {
		ObscuraBindSource(light, OBSCURA_LIGHT_SOURCE_TYPE_AMBIENT, allocator);

		evstack[evpointer].type = PARSER_STATE_TYPE_LIGHT_AMBIENT;
		evstack[evpointer].ptr  = light;
	} else if (!strcmp((char *) event->data.scalar.value, "directional")) {
		ObscuraBindSource(light, OBSCURA_LIGHT_SOURCE_TYPE_DIRECTIONAL, allocator);

		evstack[evpointer].type = PARSER_STATE_TYPE_LIGHT_DIRECTIONAL;
		evstack[evpointer].ptr  = light;
	} else if (!strcmp((char *) event->data.scalar.value, "point")) {
		ObscuraBindSource(light, OBSCURA_LIGHT_SOURCE_TYPE_POINT, allocator);

		evstack[evpointer].type = PARSER_STATE_TYPE_LIGHT_POINT;
		evstack[evpointer].ptr  = light;
	} else if (!strcmp((char *) event->data.scalar.value, "spot")) {
		ObscuraBindSource(light, OBSCURA_LIGHT_SOURCE_TYPE_SPOT, allocator);

		evstack[evpointer].type = PARSER_STATE_TYPE_LIGHT_SPOT;
		evstack[evpointer].ptr  = light;
	} else {
		assert(false);
	}
}

static void
light_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
light_ambient_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ObscuraLight *light = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "color")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR;
		evstack[evpointer].ptr = &((ObscuraLightAmbient *) light->source)->color;
	} else {
		assert(false);
	}
}

static void
light_ambient_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
light_directional_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ObscuraLight *light = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "color")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR;
		evstack[evpointer].ptr = &((ObscuraLightDirectional *) light->source)->color;
	} else if (!strcmp((char *) event->data.scalar.value, "direction")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR4;
		evstack[evpointer].ptr = &((ObscuraLightDirectional *) light->source)->direction;
	} else {
		assert(false);
	}
}

static void
light_directional_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
light_point_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ObscuraLight *light = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "color")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR;
		evstack[evpointer].ptr = &((ObscuraLightPoint *) light->source)->color;
	} else if (!strcmp((char *) event->data.scalar.value, "constant_attenuation")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
		evstack[evpointer].ptr = &((ObscuraLightPoint *) light->source)->constant_attenuation;
	} else if (!strcmp((char *) event->data.scalar.value, "linear_attenuation")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
		evstack[evpointer].ptr = &((ObscuraLightPoint *) light->source)->linear_attenuation;
	} else if (!strcmp((char *) event->data.scalar.value, "quadratic_attenuation")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
		evstack[evpointer].ptr = &((ObscuraLightPoint *) light->source)->quadratic_attenuation;
	} else {
		assert(false);
	}
}

static void
light_point_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
light_spot_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ObscuraLight *light = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "color")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR;
		evstack[evpointer].ptr = &((ObscuraLightSpot *) light->source)->color;
	} else if (!strcmp((char *) event->data.scalar.value, "direction")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR4;
		evstack[evpointer].ptr = &((ObscuraLightSpot *) light->source)->direction;
	} else if (!strcmp((char *) event->data.scalar.value, "constant_attenuation")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
		evstack[evpointer].ptr = &((ObscuraLightSpot *) light->source)->constant_attenuation;
	} else if (!strcmp((char *) event->data.scalar.value, "linear_attenuation")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
		evstack[evpointer].ptr = &((ObscuraLightSpot *) light->source)->linear_attenuation;
	} else if (!strcmp((char *) event->data.scalar.value, "quadratic_attenuation")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
		evstack[evpointer].ptr = &((ObscuraLightSpot *) light->source)->quadratic_attenuation;
	} else if (!strcmp((char *) event->data.scalar.value, "falloff_angle")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
		evstack[evpointer].ptr = &((ObscuraLightSpot *) light->source)->falloff_angle;
	} else if (!strcmp((char *) event->data.scalar.value, "falloff_exponent")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
		evstack[evpointer].ptr = &((ObscuraLightSpot *) light->source)->falloff_exponent;
	} else {
		assert(false);
	}
}

static void
light_spot_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
lights_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator)
{
	ObscuraScene *scene = evstack[evpointer].ptr;

	evpointer++;
	ObscuraComponent *light = ObscuraAcquireComponent(scene, OBSCURA_COMPONENT_FAMILY_LIGHT, allocator);
	assert(light);

	evstack[evpointer].type = PARSER_STATE_TYPE_LIGHT;
	evstack[evpointer].ptr  = light->component;

	if (event->data.scalar.anchor != NULL) {
		anchoridx++;
		strcpy(anchors[anchoridx].name, (char *) event->data.scalar.anchor);
		anchors[anchoridx].ptr = light;
	} else {
		assert(false);
	}
}

static void
lights_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
material_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator)
{
	ObscuraMaterial *material = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "constant")) {
		ObscuraBindEffect(material, OBSCURA_MATERIAL_EFFECT_TYPE_CONSTANT, allocator);

		evstack[evpointer].type = PARSER_STATE_TYPE_MATERIAL_CONSTANT;
		evstack[evpointer].ptr  = material;
	} else if (!strcmp((char *) event->data.scalar.value, "phong")) {
		ObscuraBindEffect(material, OBSCURA_MATERIAL_EFFECT_TYPE_PHONG, allocator);

		evstack[evpointer].type = PARSER_STATE_TYPE_MATERIAL_PHONG;
		evstack[evpointer].ptr  = material;
	} else {
		assert(false);
	}
}

static void
material_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
material_constant_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ObscuraMaterial *material = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "emission")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR_OR_TEXTURE;
		evstack[evpointer].ptr = &((ObscuraMaterialConstant *) material->effect)->emission;
	} else if (!strcmp((char *) event->data.scalar.value, "reflective")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR_OR_TEXTURE;
		evstack[evpointer].ptr = &((ObscuraMaterialConstant *) material->effect)->reflective;
	} else if (!strcmp((char *) event->data.scalar.value, "reflectivity")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
		evstack[evpointer].ptr = &((ObscuraMaterialConstant *) material->effect)->reflectivity;
	} else if (!strcmp((char *) event->data.scalar.value, "transparent")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR_OR_TEXTURE;
		evstack[evpointer].ptr = &((ObscuraMaterialConstant *) material->effect)->transparent;
	} else if (!strcmp((char *) event->data.scalar.value, "transparency")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
		evstack[evpointer].ptr = &((ObscuraMaterialConstant *) material->effect)->transparency;
	} else if (!strcmp((char *) event->data.scalar.value, "index_of_refraction")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
		evstack[evpointer].ptr = &((ObscuraMaterialConstant *) material->effect)->index_of_refraction;
	} else {
		assert(false);
	}
}

static void
material_constant_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
material_phong_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ObscuraMaterial *material = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "emission")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR_OR_TEXTURE;
		evstack[evpointer].ptr = &((ObscuraMaterialPhong *) material->effect)->emission;
	} else if (!strcmp((char *) event->data.scalar.value, "ambient")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR_OR_TEXTURE;
		evstack[evpointer].ptr = &((ObscuraMaterialPhong *) material->effect)->ambient;
	} else if (!strcmp((char *) event->data.scalar.value, "diffuse")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR_OR_TEXTURE;
		evstack[evpointer].ptr = &((ObscuraMaterialPhong *) material->effect)->diffuse;
	} else if (!strcmp((char *) event->data.scalar.value, "specular")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR_OR_TEXTURE;
		evstack[evpointer].ptr = &((ObscuraMaterialPhong *) material->effect)->specular;
	} else if (!strcmp((char *) event->data.scalar.value, "shininess")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR_OR_TEXTURE;
		evstack[evpointer].ptr = &((ObscuraMaterialPhong *) material->effect)->shininess;
	} else if (!strcmp((char *) event->data.scalar.value, "reflective")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR_OR_TEXTURE;
		evstack[evpointer].ptr = &((ObscuraMaterialPhong *) material->effect)->reflective;
	} else if (!strcmp((char *) event->data.scalar.value, "reflectivity")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
		evstack[evpointer].ptr = &((ObscuraMaterialPhong *) material->effect)->reflectivity;
	} else if (!strcmp((char *) event->data.scalar.value, "transparent")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR_OR_TEXTURE;
		evstack[evpointer].ptr = &((ObscuraMaterialPhong *) material->effect)->transparent;
	} else if (!strcmp((char *) event->data.scalar.value, "transparency")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
		evstack[evpointer].ptr = &((ObscuraMaterialPhong *) material->effect)->transparency;
	} else if (!strcmp((char *) event->data.scalar.value, "index_of_refraction")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
		evstack[evpointer].ptr = &((ObscuraMaterialPhong *) material->effect)->index_of_refraction;
	} else {
		assert(false);
	}
}

static void
material_phong_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
materials_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator)
{
	ObscuraScene *scene = evstack[evpointer].ptr;

	evpointer++;
	ObscuraComponent *material = ObscuraAcquireComponent(scene, OBSCURA_COMPONENT_FAMILY_MATERIAL, allocator);
	assert(material);

	evstack[evpointer].type = PARSER_STATE_TYPE_MATERIAL;
	evstack[evpointer].ptr  = material->component;

	if (event->data.scalar.anchor != NULL) {
		anchoridx++;
		strcpy(anchors[anchoridx].name, (char *) event->data.scalar.anchor);
		anchors[anchoridx].ptr = material;
	} else {
		assert(false);
	}
}

static void
materials_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
node_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ObscuraNode *node = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "components")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COMPONENTS;
		evstack[evpointer].ptr  = node;
	} else if (!strcmp((char *) event->data.scalar.value, "position")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR4;
		evstack[evpointer].ptr  = &node->position;
	} else if (!strcmp((char *) event->data.scalar.value, "interest")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR4;
		evstack[evpointer].ptr  = &node->interest;
	} else if (!strcmp((char *) event->data.scalar.value, "up")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR4;
		evstack[evpointer].ptr  = &node->up;
	} else {
		assert(false);
	}
}

static void
node_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
nodes_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator)
{
	ObscuraScene *scene = evstack[evpointer].ptr;

	evpointer++;
	ObscuraNode *node = ObscuraAcquireNode(scene, allocator);
	assert(node);

	evstack[evpointer].type = PARSER_STATE_TYPE_NODE;
	evstack[evpointer].ptr  = node;

	if (event->data.scalar.anchor != NULL) {
		anchoridx++;
		strcpy(anchors[anchoridx].name, (char *) event->data.scalar.anchor);
		anchors[anchoridx].ptr = node;
	}
}

static void
nodes_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

static void
scene_scalar_event(yaml_event_t *event, ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	ObscuraScene *scene = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "cameras")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_CAMERAS;
		evstack[evpointer].ptr  = scene;
	} else if (!strcmp((char *) event->data.scalar.value, "bounding_volumes")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_BOUNDING_VOLUMES;
		evstack[evpointer].ptr  = scene;
	} else if (!strcmp((char *) event->data.scalar.value, "materials")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_MATERIALS;
		evstack[evpointer].ptr  = scene;
	} else if (!strcmp((char *) event->data.scalar.value, "geometries")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_GEOMETRIES;
		evstack[evpointer].ptr  = scene;
	} else if (!strcmp((char *) event->data.scalar.value, "lights")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_LIGHTS;
		evstack[evpointer].ptr  = scene;
	} else if (!strcmp((char *) event->data.scalar.value, "nodes")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_NODES;
		evstack[evpointer].ptr  = scene;
	} else if (!strcmp((char *) event->data.scalar.value, "view")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_REF;
		evstack[evpointer].ptr  = &scene->view;
	} else {
		assert(false);
	}
}

static void
scene_end_event(yaml_event_t *event __attribute__((unused)), ObscuraAllocationCallbacks *allocator __attribute__((unused)))
{
	evpointer--;
}

ObscuraWorld *
ObscuraCreateWorld()
{
	ObscuraWorld *world = mem_alloc(sizeof(ObscuraWorld), 8);
	world->allocator.allocation   = &mem_alloc;
	world->allocator.reallocation = &mem_realloc;
	world->allocator.free         = &mem_free;

	world->work_queue = ObscuraCreateWorkQueue(get_nprocs(), 256, &ObscuraBusySpinWait, &world->allocator);

	return world;
}

void
ObscuraDestroyWorld(ObscuraWorld **ptr)
{
	ObscuraWorld *world = *ptr;
	ObscuraDestroyWorkQueue(&world->work_queue, &world->allocator);

	mem_free(*ptr);

	*ptr = NULL;
}

void
ObscuraLoadWorld(ObscuraWorld *world, const char *filename)
{
	yaml_parser_t parser;
	yaml_parser_initialize(&parser);

	yaml_event_t event;

	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		fprintf(stderr, "%s:%d: file not found '%s'\n", __FILE__, __LINE__, filename);
		exit(EXIT_FAILURE);
	}
	yaml_parser_set_input_file(&parser, file);

	world->scene = ObscuraCreateScene(&world->allocator);
	assert(world->scene);

	evpointer = 0;
	evstack[evpointer].type = PARSER_STATE_TYPE_SCENE;
	evstack[evpointer].ptr  = world->scene;

	do {
		if (!yaml_parser_parse(&parser, &event)) {
			fprintf(stderr, "%s:%d: %d\n", __FILE__, __LINE__, parser.error);
			exit(EXIT_FAILURE);
		}

		switch (evstack[evpointer].type) {
		case PARSER_STATE_TYPE_CAMERA:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				camera_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				camera_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_CAMERA_ANTI_ALIASING:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				camera_anti_aliasing_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				camera_anti_aliasing_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_CAMERA_PERSPECTIVE:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				camera_perspective_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				camera_perspective_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_CAMERAS:
			switch (event.type) {
			case YAML_MAPPING_START_EVENT:
				cameras_scalar_event(&event, &world->allocator);
				break;
			case YAML_SEQUENCE_END_EVENT:
				cameras_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_BOUNDING_VOLUME:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				bounding_volume_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				bounding_volume_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_BOUNDING_VOLUME_BALL:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				bounding_volume_ball_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				bounding_volume_ball_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_BOUNDING_VOLUME_BOX:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				bounding_volume_box_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				bounding_volume_box_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_BOUNDING_VOLUMES:
			switch (event.type) {
			case YAML_MAPPING_START_EVENT:
				bounding_volumes_scalar_event(&event, &world->allocator);
				break;
			case YAML_SEQUENCE_END_EVENT:
				bounding_volumes_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_COLOR:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				evpointer++;
				evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
				evstack[evpointer].ptr  = evstack[evpointer - 1].ptr;

				if (!strcmp((char *) event.data.scalar.value, "r")) {
					evstack[evpointer].ptr += 0 * sizeof(float);
				} else if (!strcmp((char *) event.data.scalar.value, "g")) {
					evstack[evpointer].ptr += 1 * sizeof(float);
				} else if (!strcmp((char *) event.data.scalar.value, "b")) {
					evstack[evpointer].ptr += 2 * sizeof(float);
				} else if (!strcmp((char *) event.data.scalar.value, "a")) {
					evstack[evpointer].ptr += 3 * sizeof(float);
				} else {
					assert(false);
				}
				break;
			case YAML_MAPPING_END_EVENT:
				evpointer--;
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_COLOR_OR_TEXTURE:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				evpointer++;
				if (!strcmp((char *) event.data.scalar.value, "color")) {
					struct __material_color_or_texture *value = evstack[evpointer - 1].ptr;
					value->type = OBSCURA_MATERIAL_VALUE_TYPE_COLOR;

					evstack[evpointer].type = PARSER_STATE_TYPE_COLOR;
					evstack[evpointer].ptr  = &value->value.color;
				} else {
					assert(false);
				}
				break;
			case YAML_MAPPING_END_EVENT:
				evpointer--;
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_COMPONENTS:
			switch (event.type) {
			case YAML_ALIAS_EVENT:
				components_scalar_event(&event, &world->allocator);
				break;
			case YAML_SEQUENCE_END_EVENT:
				components_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_FLOAT:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				*((float *) evstack[evpointer].ptr) = atof((char *) event.data.scalar.value);
				evpointer--;
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_INT:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				*((int *) evstack[evpointer].ptr) = atoi((char *) event.data.scalar.value);
				evpointer--;
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_GEOMETRY:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				geometry_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				geometry_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_GEOMETRY_SPHERE:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				geometry_sphere_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				geometry_sphere_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_GEOMETRIES:
			switch (event.type) {
			case YAML_MAPPING_START_EVENT:
				geometries_scalar_event(&event, &world->allocator);
				break;
			case YAML_SEQUENCE_END_EVENT:
				geometries_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_LIGHT:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				light_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				light_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_LIGHT_AMBIENT:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				light_ambient_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				light_ambient_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_LIGHT_DIRECTIONAL:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				light_directional_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				light_directional_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_LIGHT_POINT:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				light_point_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				light_point_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_LIGHT_SPOT:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				light_spot_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				light_spot_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_LIGHTS:
			switch (event.type) {
			case YAML_MAPPING_START_EVENT:
				lights_scalar_event(&event, &world->allocator);
				break;
			case YAML_SEQUENCE_END_EVENT:
				lights_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_MATERIAL:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				material_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				material_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_MATERIAL_CONSTANT:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				material_constant_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				material_constant_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_MATERIAL_PHONG:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				material_phong_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				material_phong_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_MATERIALS:
			switch (event.type) {
			case YAML_MAPPING_START_EVENT:
				materials_scalar_event(&event, &world->allocator);
				break;
			case YAML_SEQUENCE_END_EVENT:
				materials_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_REF:
			switch (event.type) {
			case YAML_ALIAS_EVENT:
				for (int i = 0; i < anchoridx + 1; i++) {
					if (!strcmp(anchors[i].name, (char *) event.data.alias.anchor)) {
						void **pptr = evstack[evpointer].ptr;
						*pptr = anchors[i].ptr;
						break;
					}
				}
				evpointer--;
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_NODE:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				node_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				node_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_NODES:
			switch (event.type) {
			case YAML_MAPPING_START_EVENT:
				nodes_scalar_event(&event, &world->allocator);
				break;
			case YAML_SEQUENCE_END_EVENT:
				nodes_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_VECTOR4:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				evpointer++;
				evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;
				evstack[evpointer].ptr = evstack[evpointer - 1].ptr;

				if (!strcmp((char *) event.data.scalar.value, "x")) {
					evstack[evpointer].ptr += 0 * sizeof(float);
				} else if (!strcmp((char *) event.data.scalar.value, "y")) {
					evstack[evpointer].ptr += 1 * sizeof(float);
				} else if (!strcmp((char *) event.data.scalar.value, "z")) {
					evstack[evpointer].ptr += 2 * sizeof(float);
				} else if (!strcmp((char *) event.data.scalar.value, "w")) {
					evstack[evpointer].ptr += 3 * sizeof(float);
				} else {
					assert(false);
				}
				break;
			case YAML_MAPPING_END_EVENT:
				evpointer--;
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_SCENE:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				scene_scalar_event(&event, &world->allocator);
				break;
			case YAML_MAPPING_END_EVENT:
				scene_end_event(&event, &world->allocator);
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}

		if (event.type != YAML_STREAM_END_EVENT) {
			yaml_event_delete(&event);
		}
	} while (event.type != YAML_STREAM_END_EVENT);
	assert(evpointer == -1);

	yaml_event_delete(&event);
	yaml_parser_delete(&parser);
}

void
ObscuraUnloadWorld(ObscuraWorld *world)
{
	ObscuraDestroyScene(&world->scene, &world->allocator);

	explicit_bzero(evstack, sizeof(struct parser_state));
	evpointer = -1;

	explicit_bzero(anchors, sizeof(struct parser_anchor));
	anchoridx = -1;
}
