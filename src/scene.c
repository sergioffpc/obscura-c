#include "camera.h"
#include "collision.h"
#include "geometry.h"
#include "material.h"
#include "scene.h"

static void traverse(ObscuraSceneNode *node, PFN_ObscuraSceneVisitorFunction visitor, void *arg) {
	for (uint32_t i = 0; i < node->children_count; i++) {
		ObscuraSceneNode *child = node->children[i];
		traverse(child, visitor, arg);
	}

	visitor(node, arg);
}

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
	case OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE_BALL:
		component->component = ObscuraCreateCollidable(OBSCURA_COLLIDABLE_SHAPE_TYPE_BALL, allocator);
		break;
	case OBSCURA_SCENE_COMPONENT_TYPE_GEOMETRY_SPHERE:
		component->component = ObscuraCreateGeometry(OBSCURA_GEOMETRY_TYPE_PARAMETRIC_SPHERE, allocator);
		break;
	case OBSCURA_SCENE_COMPONENT_TYPE_MATERIAL_COLOR:
		component->component = ObscuraCreateMaterial(OBSCURA_MATERIAL_TYPE_COLOR, allocator);
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
		case OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE_BALL:
			ObscuraDestroyCollidable((ObscuraCollidable **) &component->component, allocator);
			break;
		case OBSCURA_SCENE_COMPONENT_TYPE_GEOMETRY_SPHERE:
			ObscuraDestroyGeometry((ObscuraGeometry **) &component->component, allocator);
			break;
		case OBSCURA_SCENE_COMPONENT_TYPE_MATERIAL_COLOR:
			ObscuraDestroyMaterial((ObscuraMaterial **) &component->component, allocator);
			break;
		}
		allocator->free(component);

		*ptr = NULL;
	}
}

ObscuraSceneNode *ObscuraCreateNode(ObscuraAllocationCallbacks *allocator) {
	ObscuraSceneNode *node = allocator->allocation(sizeof(ObscuraSceneNode), 8);

	node->components_capacity = 64;
	node->components = allocator->allocation(sizeof(ObscuraSceneComponent *) * node->components_capacity, 8);

	node->children_capacity = 64;
	node->children = allocator->allocation(sizeof(ObscuraSceneNode *) * node->children_capacity, 8);

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
	if (node->children_count < node->children_capacity) {
		int i = node->children_count++;

		ObscuraSceneNode *child = ObscuraCreateNode(allocator);
		node->children[i] = child;

		return child;
	} else {
		return NULL;
	}
}

ObscuraSceneComponent *ObscuraAddComponent(ObscuraSceneNode *node,
					   ObscuraSceneComponentType type,
					   ObscuraAllocationCallbacks *allocator) {
	if (node->components_count < node->components_capacity) {
		int i = node->components_count++;

		ObscuraSceneComponent *component = ObscuraCreateComponent(type, allocator);
		node->components[i] = component;

		return component;
	} else {
		return NULL;
	}
}

ObscuraSceneComponent *ObscuraFindComponent(ObscuraSceneNode *node, ObscuraSceneComponentFamily family) {
	for (uint32_t i = 0; i < node->components_count; i++) {
		ObscuraSceneComponent *component = node->components[i];
		if (component->type & family) {
			return component;
		}
	}

	return NULL;
}

ObscuraScene *ObscuraCreateScene(ObscuraAllocationCallbacks *allocator) {
	ObscuraScene *scene = allocator->allocation(sizeof(ObscuraScene), 8);
	scene->nodes_capacity = 64;
	scene->nodes = allocator->allocation(sizeof(ObscuraSceneNode *) * scene->nodes_capacity, 8);

	return scene;
}

void ObscuraDestroyScene(ObscuraScene **ptr, ObscuraAllocationCallbacks *allocator) {
	if (*ptr != NULL) {
		for (uint32_t i = 0; i < (*ptr)->nodes_count; i++) {
			ObscuraSceneNode *node = (*ptr)->nodes[i];
			ObscuraDestroyNode(&node, allocator);
		}
		allocator->free((*ptr)->nodes);

		allocator->free(*ptr);

		*ptr = NULL;
	}
}

ObscuraSceneNode *ObscuraAddNode(ObscuraScene *scene, ObscuraAllocationCallbacks *allocator) {
	if (scene->nodes_count < scene->nodes_capacity) {
		int i = scene->nodes_count++;

		ObscuraSceneNode *node = ObscuraCreateNode(allocator);
		scene->nodes[i] = node;

		return node;
	} else {
		return NULL;
	}
}

void ObscuraTraverseScene(ObscuraScene *scene, PFN_ObscuraSceneVisitorFunction visitor, void *arg) {
	for (uint32_t i = 0; i < scene->nodes_count; i++) {
		ObscuraSceneNode *node = scene->nodes[i];
		traverse(node, visitor, arg);
	}
}
