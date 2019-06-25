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
#include "collidable.h"
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
		PARSER_STATE_TYPE_SCENE,
		PARSER_STATE_TYPE_NODE,
		PARSER_STATE_TYPE_NODES,
		PARSER_STATE_TYPE_PERSPECTIVE,
		PARSER_STATE_TYPE_FRUSTUM,
		PARSER_STATE_TYPE_FLOAT,
		PARSER_STATE_TYPE_VECTOR3,
		PARSER_STATE_TYPE_REF,
	}	 type;
	void	*ptr;
}		evstack[256] = {};
static int8_t	evpointer    = -1;

static struct parser_anchor {
	char	name[256];
	void	*ptr;
}		anchors[256] = {};
static int8_t	anchoridx    = -1;

static void parsescene(yaml_event_t *event) {
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

static void parsenodes(yaml_event_t *event) {
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

static void parsenode(yaml_event_t *event) {
	ObscuraSceneNode *node = evstack[evpointer].ptr;

	evpointer++;
	if (!strcmp((char *) event->data.scalar.value, "perspective")) {
		ObscuraSceneComponent *camera = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_CAMERA_PERSPECTIVE,
			&World.allocator);
		assert(camera);

		evstack[evpointer].type = PARSER_STATE_TYPE_PERSPECTIVE;
		evstack[evpointer].ptr  = camera->component;
	} else if (!strcmp((char *) event->data.scalar.value, "frustum")) {
		ObscuraSceneComponent *collidable = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE_FRUSTUM,
			&World.allocator);
		assert(collidable);

		evstack[evpointer].type = PARSER_STATE_TYPE_FRUSTUM;
		evstack[evpointer].ptr  = collidable->component;
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

static void parseperspective(yaml_event_t *event) {
	ObscuraCamera *camera = evstack[evpointer].ptr;

	evpointer++;
	evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;

	if (!strcmp((char *) event->data.scalar.value, "yfov")) {
		evstack[evpointer].ptr = &((ObscuraCameraPerspective *) camera->projection)->yfov;
	} else if (!strcmp((char *) event->data.scalar.value, "aspect_ratio")) {
		evstack[evpointer].ptr = &((ObscuraCameraPerspective *) camera->projection)->aspect_ratio;
	} else {
		assert(false);
	}
}

static void parsefrustum(yaml_event_t *event) {
	ObscuraCollidable *collidable = evstack[evpointer].ptr;

	evpointer++;
	evstack[evpointer].type = PARSER_STATE_TYPE_FLOAT;

	if (!strcmp((char *) event->data.scalar.value, "znear")) {
		evstack[evpointer].ptr = &((ObscuraCollidableFrustum *) collidable->shape)->znear;
	} else if (!strcmp((char *) event->data.scalar.value, "zfar")) {
		evstack[evpointer].ptr = &((ObscuraCollidableFrustum *) collidable->shape)->zfar;
	} else {
		assert(false);
	}
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
				parsescene(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				evpointer--;
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_NODES:
			switch (event.type) {
			case YAML_MAPPING_START_EVENT:
				parsenodes(&event);
				break;
			case YAML_SEQUENCE_END_EVENT:
				evpointer--;
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_NODE:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				parsenode(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				evpointer--;
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_PERSPECTIVE:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				parseperspective(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				evpointer--;
				break;
			default:
				break;
			}
			break;
		case PARSER_STATE_TYPE_FRUSTUM:
			switch (event.type) {
			case YAML_SCALAR_EVENT:
				parsefrustum(&event);
				break;
			case YAML_MAPPING_END_EVENT:
				evpointer--;
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
