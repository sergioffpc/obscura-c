#include "camera.h"
#include "collidable.h"
#include "scene.h"

ObscuraSceneComponent *ObscuraCreateComponent(ObscuraSceneComponentType type, ObscuraAllocationCallbacks *allocator) {
	ObscuraSceneComponent *component = allocator->allocation(sizeof(ObscuraSceneComponent), 8);
	component->type = type;

	switch (component->type) {
	case OBSCURA_SCENE_COMPONENT_TYPE_CAMERA_PERSPECTIVE:
		component->component = ObscuraCreateCamera(OBSCURA_CAMERA_PROJECTION_TYPE_PERSPECTIVE, allocator);
		break;
	case OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE_FRUSTUM:
		component->component = ObscuraCreateCollidable(OBSCURA_COLLIDABLE_SHAPE_TYPE_FRUSTUM, allocator);
		break;
	}

	return component;
}

void ObscuraDestroyComponent(ObscuraSceneComponent **ptr, ObscuraAllocationCallbacks *allocator) {
	ObscuraSceneComponent *component = *ptr;
	if (component != NULL) {
		switch (component->type) {
		case OBSCURA_SCENE_COMPONENT_TYPE_CAMERA_PERSPECTIVE:
			ObscuraDestroyCamera((ObscuraCamera **) &component->component, allocator);
			break;
		case OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE_FRUSTUM:
			ObscuraDestroyCollidable((ObscuraCollidable **) &component->component, allocator);
			break;
		}
		allocator->free(component);

		*ptr = NULL;
	}
}

ObscuraSceneNode *ObscuraCreateNode(ObscuraAllocationCallbacks *allocator) {
	ObscuraSceneNode *node = allocator->allocation(sizeof(ObscuraSceneNode), 8);

	node->components_count = 64;
	node->components = allocator->allocation(sizeof(ObscuraSceneComponent *), 8);

	node->children_count = 64;
	node->children = allocator->allocation(sizeof(ObscuraSceneNode *), 8);

	return node;
}

void ObscuraDestroyNode(ObscuraSceneNode **ptr, ObscuraAllocationCallbacks *allocator) {
	ObscuraSceneNode *node = *ptr;
	if (node != NULL) {
		for (uint32_t i = 0; i < node->components_count; i++) {
			ObscuraSceneComponent *component = node->components[i];
			ObscuraDestroyComponent(&component, allocator);
		}
		allocator->free(node->components);

		for (uint32_t i = 0; i < node->children_count; i++) {
			ObscuraSceneNode *child = node->children[i];
			ObscuraDestroyNode(&child, allocator);
		}
		allocator->free(node->children);

		allocator->free(node);

		*ptr = NULL;
	}
}

ObscuraSceneNode *ObscuraAddChild(ObscuraSceneNode *node, ObscuraAllocationCallbacks *allocator) {
	for (uint32_t i = 0; i < node->children_count; i++) {
		if (node->children[i] == NULL) {
			ObscuraSceneNode *child = ObscuraCreateNode(allocator);
			node->children[i] = child;

			return child;
		}
	}

	return NULL;
}

ObscuraSceneComponent *ObscuraAddComponent(ObscuraSceneNode *node,
					   ObscuraSceneComponentType type,
					   ObscuraAllocationCallbacks *allocator) {
	for (uint32_t i = 0; i < node->components_count; i++) {
		if (node->components[i] == NULL) {
			ObscuraSceneComponent *component = ObscuraCreateComponent(type, allocator);
			node->components[i] = component;

			return component;
		}
	}

	return NULL;
}

ObscuraSceneComponent *ObscuraFindComponent(ObscuraSceneNode *node, ObscuraSceneComponentType type) {
	for (uint32_t i = 0; i < node->components_count; i++) {
		ObscuraSceneComponent *component = node->components[i];
		if (component != NULL) {
			if (component->type == type) {
				return component;
			}
		} else {
			return NULL;
		}
	}

	return NULL;
}

ObscuraScene *ObscuraCreateScene(ObscuraAllocationCallbacks *allocator) {
	ObscuraScene *scene = allocator->allocation(sizeof(ObscuraScene), 8);
	scene->nodes_count = 64;
	scene->nodes = allocator->allocation(sizeof(ObscuraSceneNode *) * scene->nodes_count, 8);

	return scene;
}

void ObscuraDestroyScene(ObscuraScene **ptr, ObscuraAllocationCallbacks *allocator) {
	allocator->free(*ptr);

	*ptr = NULL;
}

ObscuraSceneNode *ObscuraAddNode(ObscuraScene *scene, ObscuraAllocationCallbacks *allocator) {
	for (uint32_t i = 0; i < scene->nodes_count; i++) {
		if (scene->nodes[i] == NULL) {
			ObscuraSceneNode *node = ObscuraCreateNode(allocator);
			scene->nodes[i] = node;

			return node;
		}
	}

	return NULL;
}
