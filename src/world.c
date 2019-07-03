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
#include "world.h"

static void mem_free(void *ptr) {
	free(ptr);
}

static void *mem_alloc(size_t size, size_t alignment) {
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

static void *mem_realloc(void *original, size_t size, size_t alignment) {
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

ObscuraWorld World = {
	.allocator = {
		.allocation   = &mem_alloc,
		.reallocation = &mem_realloc,
		.free         = &mem_free,
	},
};

static struct parser_state {
	enum {
		PARSER_STATE_TYPE_CAMERA,
		PARSER_STATE_TYPE_CAMERA_PERSPECTIVE,
		PARSER_STATE_TYPE_COLLISION,
		PARSER_STATE_TYPE_COLLISION_BALL,
		PARSER_STATE_TYPE_COLLISION_BOX,
		PARSER_STATE_TYPE_FLOAT,
		PARSER_STATE_TYPE_GEOMETRY,
		PARSER_STATE_TYPE_GEOMETRY_SPHERE,
		PARSER_STATE_TYPE_LIGHT,
		PARSER_STATE_TYPE_LIGHT_AMBIENT,
		PARSER_STATE_TYPE_LIGHT_DIRECTIONAL,
		PARSER_STATE_TYPE_LIGHT_POINT,
		PARSER_STATE_TYPE_LIGHT_SPOT,
		PARSER_STATE_TYPE_MATERIAL,
		PARSER_STATE_TYPE_REF,
		PARSER_STATE_TYPE_SCENE,
		PARSER_STATE_TYPE_SCENE_NODE,
		PARSER_STATE_TYPE_SCENE_NODES,
		PARSER_STATE_TYPE_VECTOR3,
	}	 type;
	void	*ptr;
}		evstack[256] = {};
static int8_t	evpointer    = -1;

static struct parser_anchor {
	char	*name;
	void	*ptr;
}		anchors[256] = {};
static int8_t	anchoridx    = -1;

static void scene_scalar_event(yaml_event_t *event) {
	ObscuraScene *scene = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "nodes")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_SCENE_NODES;
		evstack[evpointer].ptr  = scene;
	} else if (!strcmp((char *) event->data.scalar.value, "view")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_REF;
		evstack[evpointer].ptr  = &scene->view;
	} else {
		assert(false);
	}
}

static void scene_end_event(yaml_event_t *event __attribute__((unused))) {
	evpointer--;
}

static void scene_node_scalar_event(yaml_event_t *event) {
	ObscuraSceneNode *node = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "camera")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_CAMERA;
		evstack[evpointer].ptr  = node;
	} else if (!strcmp((char *) event->data.scalar.value, "collision")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_COLLISION;
		evstack[evpointer].ptr  = node;
	} else if (!strcmp((char *) event->data.scalar.value, "geometry")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_GEOMETRY;
		evstack[evpointer].ptr  = node;
	} else if (!strcmp((char *) event->data.scalar.value, "material")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_MATERIAL;
		evstack[evpointer].ptr  = node;
	} else if (!strcmp((char *) event->data.scalar.value, "light")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_LIGHT;
		evstack[evpointer].ptr  = node;
	} else if (!strcmp((char *) event->data.scalar.value, "position")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR3;
		evstack[evpointer].ptr  = &node->position;
	} else if (!strcmp((char *) event->data.scalar.value, "interest")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR3;
		evstack[evpointer].ptr  = &node->interest;
	} else if (!strcmp((char *) event->data.scalar.value, "up")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR3;
		evstack[evpointer].ptr  = &node->up;
	} else {
		assert(false);
	}
}

static void scene_node_end_event(yaml_event_t *event __attribute__((unused))) {
	evpointer--;
}

static void scene_nodes_scalar_event(yaml_event_t *event) {
	ObscuraScene *scene = evstack[evpointer].ptr;

	evpointer++;
	ObscuraSceneNode *node = ObscuraAddNode(scene, &World.allocator);
	assert(node);

	evstack[evpointer].type = PARSER_STATE_TYPE_SCENE_NODE;
	evstack[evpointer].ptr  = node;

	if (event->data.scalar.anchor != NULL) {
		anchoridx++;
		strcpy(node->name, (char *) event->data.scalar.anchor);
		anchors[anchoridx].name = node->name;
		anchors[anchoridx].ptr = evstack[evpointer].ptr;
	}
}

static void scene_nodes_end_event(yaml_event_t *event __attribute__((unused))) {
	evpointer--;
}

static void camera_scalar_event(yaml_event_t *event) {
	ObscuraSceneNode *node = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "perspective")) {
		ObscuraSceneComponent *camera = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_CAMERA_PERSPECTIVE,
			&World.allocator);
		assert(camera);

		evstack[evpointer].type = PARSER_STATE_TYPE_CAMERA_PERSPECTIVE;
		evstack[evpointer].ptr  = camera->component;
	}
}

static void camera_end_event(yaml_event_t *event __attribute__((unused))) {
	evpointer--;
}

static void camera_perspective_scalar_event(yaml_event_t *event) {
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

static void camera_perspective_end_event(yaml_event_t *event __attribute__((unused))) {
	ObscuraCamera *camera = evstack[evpointer].ptr;
	ObscuraCameraPerspective *perspective = camera->projection;

	evpointer--;
	ObscuraSceneNode *node = evstack[evpointer].ptr;
	ObscuraSceneComponent *component = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE_FRUSTUM,
		&World.allocator);
	ObscuraCollidable *collidable = component->component;
	ObscuraCollidableFrustum *frustum = collidable->shape;
	frustum->bottom = 0;
	frustum->left   = 0;
	frustum->right  = 0;
	frustum->top    = 0;
	frustum->near   = perspective->znear;
	frustum->far    = perspective->zfar;
}

static void collision_scalar_event(yaml_event_t *event) {
	ObscuraSceneNode *node = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "ball")) {
		ObscuraSceneComponent *collidable = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE_BALL,
			&World.allocator);
		assert(collidable);

		evstack[evpointer].type = PARSER_STATE_TYPE_COLLISION_BALL;
		evstack[evpointer].ptr  = collidable->component;
	} else if (!strcmp((char *) event->data.scalar.value, "box")) {
		ObscuraSceneComponent *collidable = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE_BOX,
			&World.allocator);
		assert(collidable);

		evstack[evpointer].type = PARSER_STATE_TYPE_COLLISION_BOX;
		evstack[evpointer].ptr  = collidable->component;
	} else {
		assert(false);
	}
}

static void collision_end_event(yaml_event_t *event __attribute__((unused))) {
	evpointer--;
}

static void collision_ball_scalar_event(yaml_event_t *event) {
	ObscuraCollidable *collidable = evstack[evpointer].ptr;

	evpointer++;
	evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;

	if (!strcmp((char *) event->data.scalar.value, "radius")) {
		evstack[evpointer].ptr = &((ObscuraCollidableBall *) collidable->shape)->radius;
	} else {
		assert(false);
	}
}

static void collision_ball_end_event(yaml_event_t *event __attribute__((unused))) {
	evpointer--;
}

static void collision_box_scalar_event(yaml_event_t *event) {
	ObscuraCollidable *collidable = evstack[evpointer].ptr;

	evpointer++;
	evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR3;

	if (!strcmp((char *) event->data.scalar.value, "half_extents")) {
		evstack[evpointer].ptr = &((ObscuraCollidableBox *) collidable->shape)->half_extents;
	} else {
		assert(false);
	}
}

static void collision_box_end_event(yaml_event_t *event __attribute__((unused))) {
	evpointer--;
}

static void geometry_scalar_event(yaml_event_t *event) {
	ObscuraSceneNode *node = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "sphere")) {
		ObscuraSceneComponent *geometry = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_GEOMETRY_SPHERE,
			&World.allocator);
		assert(geometry);

		evstack[evpointer].type = PARSER_STATE_TYPE_GEOMETRY_SPHERE;
		evstack[evpointer].ptr  = geometry->component;
	}
}

static void geometry_end_event(yaml_event_t *event __attribute__((unused))) {
	evpointer--;
}

static void geometry_sphere_scalar_event(yaml_event_t *event) {
	ObscuraGeometry *geometry = evstack[evpointer].ptr;

	evpointer++;
	evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;

	if (!strcmp((char *) event->data.scalar.value, "radius")) {
		evstack[evpointer].ptr = &((ObscuraGeometrySphere *) geometry->geometry)->radius;
	} else {
		assert(false);
	}
}

static void geometry_sphere_end_event(yaml_event_t *event __attribute__((unused))) {
	evpointer--;
}

static void light_scalar_event(yaml_event_t *event) {
	ObscuraSceneNode *node = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "ambient")) {
		ObscuraSceneComponent *light = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_LIGHT_AMBIENT,
			&World.allocator);
		assert(light);

		evstack[evpointer].type = PARSER_STATE_TYPE_LIGHT_AMBIENT;
		evstack[evpointer].ptr  = light->component;
	} else if (!strcmp((char *) event->data.scalar.value, "directional")) {
		ObscuraSceneComponent *light = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_LIGHT_DIRECTIONAL,
			&World.allocator);
		assert(light);

		evstack[evpointer].type = PARSER_STATE_TYPE_LIGHT_DIRECTIONAL;
		evstack[evpointer].ptr  = light->component;
	} else if (!strcmp((char *) event->data.scalar.value, "point")) {
		ObscuraSceneComponent *light = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_LIGHT_POINT,
			&World.allocator);
		assert(light);

		evstack[evpointer].type = PARSER_STATE_TYPE_LIGHT_POINT;
		evstack[evpointer].ptr  = light->component;
	} else if (!strcmp((char *) event->data.scalar.value, "spot")) {
		ObscuraSceneComponent *light = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_LIGHT_SPOT,
			&World.allocator);
		assert(light);

		evstack[evpointer].type = PARSER_STATE_TYPE_LIGHT_SPOT;
		evstack[evpointer].ptr  = light->component;
	}
}

static void light_end_event(yaml_event_t *event __attribute__((unused))) {
	evpointer--;
}

static void light_ambient_scalar_event(yaml_event_t *event) {
	ObscuraLight *light = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "color")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR3;
		evstack[evpointer].ptr = &((ObscuraLightAmbient *) light->source)->color;
	} else {
		assert(false);
	}
}

static void light_ambient_end_event(yaml_event_t *event __attribute__((unused))) {
	evpointer--;
}

static void light_directional_scalar_event(yaml_event_t *event) {
	ObscuraLight *light = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "color")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR3;
		evstack[evpointer].ptr = &((ObscuraLightDirectional *) light->source)->color;
	} else if (!strcmp((char *) event->data.scalar.value, "direction")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR3;
		evstack[evpointer].ptr = &((ObscuraLightDirectional *) light->source)->direction;
	} else {
		assert(false);
	}
}

static void light_directional_end_event(yaml_event_t *event __attribute__((unused))) {
	evpointer--;
}

static void light_point_scalar_event(yaml_event_t *event) {
	ObscuraLight *light = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "color")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR3;
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

static void light_point_end_event(yaml_event_t *event __attribute__((unused))) {
	ObscuraLight *light = evstack[evpointer].ptr;
	ObscuraLightPoint *point = light->source;

	evpointer--;
	ObscuraSceneNode *node = evstack[evpointer].ptr;
	ObscuraSceneComponent *component = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE_BALL,
		&World.allocator);
	ObscuraCollidable *collidable = component->component;
	ObscuraCollidableBall *ball = collidable->shape;
	ball->radius = 0;
}

static void light_spot_scalar_event(yaml_event_t *event) {
	ObscuraLight *light = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "color")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR3;
		evstack[evpointer].ptr = &((ObscuraLightSpot *) light->source)->color;
	} else if (!strcmp((char *) event->data.scalar.value, "direction")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR3;
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

static void light_spot_end_event(yaml_event_t *event __attribute__((unused))) {
	ObscuraLight *light = evstack[evpointer].ptr;
	ObscuraLightSpot *spot = light->source;

	evpointer--;
	ObscuraSceneNode *node = evstack[evpointer].ptr;
	ObscuraSceneComponent *component = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE_CONE,
		&World.allocator);
	ObscuraCollidable *collidable = component->component;
	ObscuraCollidableCone *cone = collidable->shape;
	cone->height = 0;
	cone->radius = 0;
}

static void material_scalar_event(yaml_event_t *event) {
	ObscuraSceneNode *node = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "color")) {
		ObscuraSceneComponent *material = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_MATERIAL_COLOR,
			&World.allocator);
		assert(material);

		ObscuraMaterial *color = material->component;

		evstack[evpointer].type = PARSER_STATE_TYPE_VECTOR3;
		evstack[evpointer].ptr  = &((ObscuraMaterialColor *) color->material)->color;
	}
}

static void material_end_event(yaml_event_t *event __attribute__((unused))) {
	evpointer--;
}

void ObscuraLoadWorld(const char *filename) {
	yaml_parser_t parser;
	yaml_parser_initialize(&parser);

	yaml_event_t event;

	FILE *file = fopen(filename, "rb");
	if (file == NULL) {
		fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, strerror(errno));
		exit(EXIT_FAILURE);
	}
	yaml_parser_set_input_file(&parser, file);

	ObscuraUnloadWorld();

	World.scene = ObscuraCreateScene(&World.allocator);
	assert(World.scene);

	evpointer = 0;
	evstack[evpointer].type = PARSER_STATE_TYPE_SCENE;
	evstack[evpointer].ptr  = World.scene;

	do {
		if (!yaml_parser_parse(&parser, &event)) {
			fprintf(stderr, "%s:%d: %d\n", __FILE__, __LINE__, parser.error);
			exit(EXIT_FAILURE);
		}

		switch (evstack[evpointer].type) {
		case PARSER_STATE_TYPE_SCENE:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				scene_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				scene_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_SCENE_NODES:
			switch (event.type) {
			case YAML_MAPPING_START_EVENT:
				scene_nodes_scalar_event(&event);
				break;
			case YAML_SEQUENCE_END_EVENT:
				scene_nodes_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_SCENE_NODE:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				scene_node_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				scene_node_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_CAMERA:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				camera_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				camera_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_CAMERA_PERSPECTIVE:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				camera_perspective_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				camera_perspective_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_COLLISION:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				collision_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				collision_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_COLLISION_BALL:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				collision_ball_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				collision_ball_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_COLLISION_BOX:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				collision_box_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				collision_box_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_GEOMETRY:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				geometry_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				geometry_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_GEOMETRY_SPHERE:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				geometry_sphere_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				geometry_sphere_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_MATERIAL:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				material_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				material_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_LIGHT:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				light_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				light_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_LIGHT_AMBIENT:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				light_ambient_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				light_ambient_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_LIGHT_DIRECTIONAL:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				light_directional_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				light_directional_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_LIGHT_POINT:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				light_point_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				light_point_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_LIGHT_SPOT:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				light_spot_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				light_spot_end_event(&event);
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
		case PARSER_STATE_TYPE_VECTOR3:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				*((float *) evstack[evpointer].ptr) = atof((char *) event.data.scalar.value);
				evstack[evpointer].ptr += sizeof(float);
				break;
			case YAML_SEQUENCE_END_EVENT:
				*((float *) evstack[evpointer].ptr) = 1;
				evpointer--;
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
		}

		if (event.type != YAML_STREAM_END_EVENT) {
			yaml_event_delete(&event);
		}
	} while (event.type != YAML_STREAM_END_EVENT);

	assert(evpointer == -1);

	yaml_event_delete(&event);
	yaml_parser_delete(&parser);
}

void ObscuraUnloadWorld() {
	ObscuraDestroyScene(&World.scene, &World.allocator);

	explicit_bzero(evstack, sizeof(struct parser_state));
	evpointer = -1;

	explicit_bzero(anchors, sizeof(struct parser_anchor));
	anchoridx = -1;
}
