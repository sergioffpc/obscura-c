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
		PARSER_STATE_OBJECT_TYPE_SCENE = 1,
		PARSER_STATE_OBJECT_TYPE_NODE,
		PARSER_STATE_OBJECT_TYPE_COMPONENT,
		PARSER_STATE_COMPONENT_TYPE_PERSPECTIVE,
		PARSER_STATE_COMPONENT_TYPE_FRUSTUM,
	}		 type;
	yaml_event_t	*event;
	void		*ptr;
}		evstack[256] = {};
static uint8_t	evpointer    = 0;

static void parsescene(yaml_event_t *event) {
	if (!strcmp((char *) event->data.scalar.value, "nodes")) {
		evpointer++;
		evstack[evpointer].type  = PARSER_STATE_OBJECT_TYPE_NODE;
		evstack[evpointer].event = event;
		evstack[evpointer].ptr   = evstack[evpointer - 1].ptr;
	} else if (!strcmp((char *) event->data.scalar.value, "view")) {
	}
}

static void parsenode(yaml_event_t *event) {
	if (!strcmp((char *) event->data.scalar.value, "node")) {
		ObscuraScene *scene = evstack[evpointer].ptr;

		ObscuraSceneNode *node = ObscuraAddNode(scene, &World.allocator);
		assert(node);

		evpointer++;
		evstack[evpointer].type  = PARSER_STATE_OBJECT_TYPE_NODE;
		evstack[evpointer].event = event;
		evstack[evpointer].ptr   = node;
	} else if (!strcmp((char *) event->data.scalar.value, "components")) {
		evpointer++;
		evstack[evpointer].type  = PARSER_STATE_OBJECT_TYPE_COMPONENT;
		evstack[evpointer].event = event;
		evstack[evpointer].ptr   = evstack[evpointer - 1].ptr;
	} else if (!strcmp((char *) event->data.scalar.value, "transformation")) {
	}
}

static void parsecomponent(yaml_event_t *event) {
	ObscuraSceneNode *node = evstack[evpointer].ptr;
	if (!strcmp((char *) event->data.scalar.value, "perspective")) {
		ObscuraSceneComponent *camera = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_CAMERA,
			&World.allocator);
		assert(camera);

		camera->component = ObscuraCreateCamera(OBSCURA_CAMERA_PROJECTION_TYPE_PERSPECTIVE, &World.allocator);

		evpointer++;
		evstack[evpointer].type  = PARSER_STATE_COMPONENT_TYPE_PERSPECTIVE;
		evstack[evpointer].event = event;
		evstack[evpointer].ptr   = camera->component;
	} else if (!strcmp((char *) event->data.scalar.value, "frustum")) {
		ObscuraSceneComponent *collidable = ObscuraAddComponent(node, OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE,
			&World.allocator);
		assert(collidable);

		collidable->component = ObscuraCreateCollidable(OBSCURA_COLLIDABLE_SHAPE_TYPE_FRUSTUM, &World.allocator);

		evpointer++;
		evstack[evpointer].type  = PARSER_STATE_COMPONENT_TYPE_FRUSTUM;
		evstack[evpointer].event = event;
		evstack[evpointer].ptr   = collidable->component;
	}
}

static void parseperspective(yaml_event_t *event) {
	ObscuraCamera *camera = evstack[evpointer].ptr;

	if (!strcmp((char *) event->data.scalar.value, "yfov")) {
		evpointer++;
		evstack[evpointer].type  = PARSER_STATE_COMPONENT_TYPE_PERSPECTIVE;
		evstack[evpointer].event = event;
		evstack[evpointer].ptr   = &((ObscuraCameraPerspective *) camera->projection)->yfov;
	} else if (!strcmp((char *) event->data.scalar.value, "aspect_ratio")) {
		evpointer++;
		evstack[evpointer].type  = PARSER_STATE_COMPONENT_TYPE_PERSPECTIVE;
		evstack[evpointer].event = event;
		evstack[evpointer].ptr   = &((ObscuraCameraPerspective *) camera->projection)->aspect_ratio;
	} else {
		*((float *) evstack[evpointer].ptr) = atof((char *) event->data.scalar.value);

		yaml_event_delete(evstack[evpointer].event);
		evpointer--;
	}
}

static void parsefrustum(yaml_event_t *event) {
	ObscuraCollidable *collidable = evstack[evpointer].ptr;

	evpointer++;
	evstack[evpointer].type  = PARSER_STATE_COMPONENT_TYPE_FRUSTUM;
	evstack[evpointer].event = event;

	if (!strcmp((char *) event->data.scalar.value, "znear")) {
		evstack[evpointer].ptr = &((ObscuraCollidableFrustum *) collidable->shape)->znear;
	} else if (!strcmp((char *) event->data.scalar.value, "zfar")) {
		evstack[evpointer].ptr = &((ObscuraCollidableFrustum *) collidable->shape)->zfar;
	} else {
		*((float *) evstack[evpointer].ptr) = atof((char *) event->data.scalar.value);

		yaml_event_delete(evstack[evpointer].event);
		evpointer--;
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

	do {
		if (!yaml_parser_parse(&parser, &event)) {
			fprintf(stderr, "%s:%d: %d\n", __FILE__, __LINE__, parser.error);
			exit(EXIT_FAILURE);
		}

		switch (event.type) {
		case YAML_NO_EVENT:
			yaml_event_delete(&event);
			break;

		case YAML_STREAM_START_EVENT:
			World.scene = ObscuraCreateScene(&World.allocator);
			assert(World.scene);

			evpointer = 0;
			evstack[evpointer].type  = PARSER_STATE_OBJECT_TYPE_SCENE;
                        evstack[evpointer].event = &event;
                        evstack[evpointer].ptr   = World.scene;
			break;
		case YAML_STREAM_END_EVENT:
			break;

		case YAML_DOCUMENT_START_EVENT:
		case YAML_DOCUMENT_END_EVENT:
			yaml_event_delete(&event);
			break;

		case YAML_ALIAS_EVENT:
			yaml_event_delete(&event);
			break;
		case YAML_SCALAR_EVENT:
			switch (evstack[evpointer].type) {
			case PARSER_STATE_OBJECT_TYPE_SCENE:
				parsescene(&event);
				break;
			case PARSER_STATE_OBJECT_TYPE_NODE:
				parsenode(&event);
				break;
			case PARSER_STATE_OBJECT_TYPE_COMPONENT:
				parsecomponent(&event);
				break;
			case PARSER_STATE_COMPONENT_TYPE_PERSPECTIVE:
				parseperspective(&event);
				break;
			case PARSER_STATE_COMPONENT_TYPE_FRUSTUM:
				parsefrustum(&event);
				break;
			}
			break;

		case YAML_SEQUENCE_START_EVENT:
		case YAML_MAPPING_START_EVENT:
			yaml_event_delete(&event);
			break;

		case YAML_SEQUENCE_END_EVENT:
		case YAML_MAPPING_END_EVENT:
			yaml_event_delete(evstack[evpointer].event);
			evpointer--;

			yaml_event_delete(&event);
			break;
		}
	} while (event.type != YAML_STREAM_END_EVENT);

	assert(evpointer == 0);

	yaml_event_delete(&event);
	yaml_parser_delete(&parser);
}

void ObscuraUnloadWorld() {
	ObscuraDestroyScene(&World.scene, &World.allocator);
}
