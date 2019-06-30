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
		PARSER_STATE_TYPE_BALL,
		PARSER_STATE_TYPE_COLOR,
		PARSER_STATE_TYPE_FLOAT,
		PARSER_STATE_TYPE_NODE,
		PARSER_STATE_TYPE_NODES,
		PARSER_STATE_TYPE_PERSPECTIVE,
		PARSER_STATE_TYPE_REF,
		PARSER_STATE_TYPE_SCENE,
		PARSER_STATE_TYPE_SPHERE,
		PARSER_STATE_TYPE_UINT32,
		PARSER_STATE_TYPE_VECTOR3,
	}	 type;
	void	*ptr;
}		evstack[256] = {};
static int8_t	evpointer    = -1;

static struct parser_anchor {
	char	name[256];
	void	*ptr;
}		anchors[256] = {};
static int8_t	anchoridx    = -1;

static void scene_scalar_event(yaml_event_t *event) {
	ObscuraScene *scene = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "nodes")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_NODES;
		evstack[evpointer].ptr  = scene;
	} else if (!strcmp((char *) event->data.scalar.value, "view")) {
		evstack[evpointer].type = PARSER_STATE_TYPE_REF;
		evstack[evpointer].ptr  = &scene->view;
	} else {
		assert(false);
	}
}

static void scene_end_event(yaml_event_t *event) {
	evpointer--;
}

static void nodes_scalar_event(yaml_event_t *event) {
	ObscuraScene *scene = evstack[evpointer].ptr;

	evpointer++;
	ObscuraSceneNode *node = ObscuraAddNode(scene, &World.allocator);
	assert(node);

	evstack[evpointer].type = PARSER_STATE_TYPE_NODE;
	evstack[evpointer].ptr  = node;

	if (event->data.scalar.anchor != NULL) {
		anchoridx++;
		strcpy(anchors[anchoridx].name, (char *) event->data.scalar.anchor);
		anchors[anchoridx].ptr = evstack[evpointer].ptr;
	}
}

static void nodes_end_event(yaml_event_t *event) {
	evpointer--;
}

static void node_scalar_event(yaml_event_t *event) {
	ObscuraSceneNode *node = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "perspective")) {
		ObscuraSceneComponent *camera = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_CAMERA_PERSPECTIVE,
			&World.allocator);
		assert(camera);

		evstack[evpointer].type = PARSER_STATE_TYPE_PERSPECTIVE;
		evstack[evpointer].ptr  = camera->component;
	} else if (!strcmp((char *) event->data.scalar.value, "ball")) {
		ObscuraSceneComponent *collidable = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE_BALL,
			&World.allocator);
		assert(collidable);

		evstack[evpointer].type = PARSER_STATE_TYPE_BALL;
		evstack[evpointer].ptr  = collidable->component;
	} else if (!strcmp((char *) event->data.scalar.value, "sphere")) {
		ObscuraSceneComponent *geometry = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_GEOMETRY_SPHERE,
			&World.allocator);
		assert(geometry);

		evstack[evpointer].type = PARSER_STATE_TYPE_SPHERE;
		evstack[evpointer].ptr  = geometry->component;
	} else if (!strcmp((char *) event->data.scalar.value, "color")) {
		ObscuraSceneComponent *material = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_MATERIAL_COLOR,
			&World.allocator);
		assert(material);

		evstack[evpointer].type = PARSER_STATE_TYPE_COLOR;
		evstack[evpointer].ptr  = material->component;
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

static void node_end_event(yaml_event_t *event) {
	evpointer--;
}

static void perspective_scalar_event(yaml_event_t *event) {
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

static void perspective_end_event(yaml_event_t *event) {
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
	frustum->near   = 0;
	frustum->far    = 0;
}

static void ball_scalar_event(yaml_event_t *event) {
	ObscuraCollidable *collidable = evstack[evpointer].ptr;

	evpointer++;
	evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;

	if (!strcmp((char *) event->data.scalar.value, "radius")) {
		evstack[evpointer].ptr = &((ObscuraCollidableBall *) collidable->shape)->radius;
	} else {
		assert(false);
	}
}

static void ball_end_event(yaml_event_t *event) {
	evpointer--;
}

static void color_scalar_event(yaml_event_t *event) {
	ObscuraMaterial *material = evstack[evpointer].ptr;

	evpointer++;
	evstack[evpointer].type = PARSER_STATE_TYPE_UINT32;
	evstack[evpointer].ptr  = &((ObscuraMaterialColor *) material->material)->color;
}

static void color_end_event(yaml_event_t *event) {
	evpointer--;
}

static void sphere_scalar_event(yaml_event_t *event) {
	ObscuraGeometry *geometry = evstack[evpointer].ptr;

	evpointer++;
	evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;

	if (!strcmp((char *) event->data.scalar.value, "radius")) {
		evstack[evpointer].ptr = &((ObscuraGeometrySphere *) geometry->geometry)->radius;
	} else {
		assert(false);
	}
}

static void sphere_end_event(yaml_event_t *event) {
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
		case PARSER_STATE_TYPE_NODES:
			switch (event.type) {
			case YAML_MAPPING_START_EVENT:
				nodes_scalar_event(&event);
				break;
			case YAML_SEQUENCE_END_EVENT:
				nodes_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_NODE:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				node_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				node_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_PERSPECTIVE:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				perspective_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				perspective_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_BALL:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				ball_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				ball_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_COLOR:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				color_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				color_end_event(&event);
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_SPHERE:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				sphere_scalar_event(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				sphere_end_event(&event);
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
		case PARSER_STATE_TYPE_UINT32:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				*((uint32_t *) evstack[evpointer].ptr) = atol((char *) event.data.scalar.value);
				evpointer--;
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_VECTOR3:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				*((float *) evstack[evpointer].ptr++) = atof((char *) event.data.scalar.value);
				break;
			case YAML_SEQUENCE_END_EVENT:
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
