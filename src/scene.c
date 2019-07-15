#include <assert.h>
#include <stdbool.h>

#include "camera.h"
#include "collision.h"
#include "geometry.h"
#include "light.h"
#include "material.h"
#include "scene.h"

static void
traverse(ObscuraNode *node, PFN_ObscuraSceneVisitorFunction visitor, void *arg, ObscuraAllocationCallbacks *allocator)
{
	for (uint32_t i = 0; i < node->children_count; i++) {
		ObscuraNode *child = node->children[i];
		traverse(child, visitor, arg, allocator);
	}

	visitor(node, arg, allocator);
}

ObscuraComponent *
ObscuraCreateComponent(ObscuraComponentFamily family, ObscuraAllocationCallbacks *allocator)
{
	ObscuraComponent *component = allocator->allocation(sizeof(ObscuraComponent), 8);
	component->family = family;

	switch (component->family) {
	case OBSCURA_COMPONENT_FAMILY_CAMERA:
		component->component = ObscuraCreateCamera(allocator);
		break;
	case OBSCURA_COMPONENT_FAMILY_BOUNDING_VOLUME:
		component->component = ObscuraCreateBoundingVolume(allocator);
		break;
	case OBSCURA_COMPONENT_FAMILY_GEOMETRY:
		component->component = ObscuraCreateGeometry(allocator);
		break;
	case OBSCURA_COMPONENT_FAMILY_LIGHT:
		component->component = ObscuraCreateLight(allocator);
		break;
	case OBSCURA_COMPONENT_FAMILY_MATERIAL:
		component->component = ObscuraCreateMaterial(allocator);
		break;
	default:
		assert(false);
		break;
	}

	return component;
}

void
ObscuraDestroyComponent(ObscuraComponent **ptr, ObscuraAllocationCallbacks *allocator)
{
	ObscuraComponent *component = *ptr;

	allocator->free(component->component);
	allocator->free(component);

	*ptr = NULL;
}

ObscuraNode *
ObscuraCreateNode(ObscuraAllocationCallbacks *allocator)
{
	ObscuraNode *node = allocator->allocation(sizeof(ObscuraNode), 8);

	node->components_capacity = 64;
	node->components = allocator->allocation(sizeof(ObscuraComponent *) * node->components_capacity, 8);

	node->children_capacity = 64;
	node->children = allocator->allocation(sizeof(ObscuraNode *) * node->children_capacity, 8);

	return node;
}

void
ObscuraDestroyNode(ObscuraNode **ptr, ObscuraAllocationCallbacks *allocator)
{
	ObscuraNode *node = *ptr;
	if (node != NULL) {
		for (uint32_t i = 0; i < node->components_count; i++) {
			ObscuraComponent *component = node->components[i];
			ObscuraDetachComponent(node, component);
		}
		allocator->free(node->components);

		for (uint32_t i = 0; i < node->children_count; i++) {
			ObscuraNode *child = node->children[i];
			ObscuraDetachChild(node, child);
		}
		allocator->free(node->children);

		allocator->free(node);

		*ptr = NULL;
	}
}

ObscuraNode *
ObscuraAttachComponent(ObscuraNode *node, ObscuraComponent *component)
{
	if (node->components_count < node->components_capacity) {
		node->components[node->components_count] = component;
		node->components_count++;
	} else {
		assert(false);
	}

	return node;
}

void
ObscuraDetachComponent(ObscuraNode *node, ObscuraComponent *component)
{
	for (uint32_t i = 0; i < node->components_count; i++) {
		if (node->components[i] == component) {
			node->components[i] = node->components[node->components_count - 1];
			node->components_count--;
			break;
		}
	}
}

ObscuraComponent *
ObscuraFindComponent(ObscuraNode *node, uint32_t family, uint32_t type)
{
	for (uint32_t i = 0; i < node->components_count; i++) {
		ObscuraComponent *component = node->components[i];
		if (component->family == family) {
			switch (component->family) {
			case OBSCURA_COMPONENT_FAMILY_CAMERA:
				if (((ObscuraCamera *) component->component)->type == type) {
					return component;
				}
				break;
			case OBSCURA_COMPONENT_FAMILY_BOUNDING_VOLUME:
				if (((ObscuraBoundingVolume *) component->component)->type == type) {
					return component;
				}
				break;
			case OBSCURA_COMPONENT_FAMILY_GEOMETRY:
				if (((ObscuraGeometry *) component->component)->type == type) {
					return component;
				}
				break;
			case OBSCURA_COMPONENT_FAMILY_LIGHT:
				if (((ObscuraLight *) component->component)->type == type) {
					return component;
				}
				break;
			case OBSCURA_COMPONENT_FAMILY_MATERIAL:
				if (((ObscuraMaterial *) component->component)->type == type) {
					return component;
				}
				break;
			default:
				assert(false);
				break;
			}
		}
	}

	return NULL;
}

ObscuraComponent *
ObscuraFindAnyComponent(ObscuraNode *node, uint32_t family)
{
	for (uint32_t i = 0; i < node->components_count; i++) {
		ObscuraComponent *component = node->components[i];
		if (component->family == family) {
			return component;
		}
	}

	return NULL;
}

ObscuraNode *
ObscuraAttachChild(ObscuraNode *node, ObscuraNode *child)
{
	if (node->children_count < node->children_capacity) {
		node->children[node->children_count] = child;
		node->children_count++;
	} else {
		assert(false);
	}

	return node;
}

void
ObscuraDetachChild(ObscuraNode *node, ObscuraNode *child)
{
	for (uint32_t i = 0; i < node->children_count; i++) {
		if (node->children[i] == child) {
			node->children[i] = node->children[node->children_count - 1];
			node->children_count--;
			break;
		}
	}
}

ObscuraScene *
ObscuraCreateScene(ObscuraAllocationCallbacks *allocator)
{
	ObscuraScene *scene = allocator->allocation(sizeof(ObscuraScene), 8);

	scene->cameras_capacity = 64;
	scene->cameras = allocator->allocation(sizeof(ObscuraComponent *) * scene->cameras_capacity, 8);

	scene->bounding_volumes_capacity = 64;
	scene->bounding_volumes = allocator->allocation(sizeof(ObscuraComponent *) * scene->bounding_volumes_capacity, 8);

	scene->geometries_capacity = 64;
	scene->geometries = allocator->allocation(sizeof(ObscuraComponent *) * scene->geometries_capacity, 8);

	scene->lights_capacity = 64;
	scene->lights = allocator->allocation(sizeof(ObscuraComponent *) * scene->lights_capacity, 8);

	scene->materials_capacity = 64;
	scene->materials = allocator->allocation(sizeof(ObscuraComponent *) * scene->materials_capacity, 8);

	scene->nodes_capacity = 64;
	scene->nodes = allocator->allocation(sizeof(ObscuraNode *) * scene->nodes_capacity, 8);

	return scene;
}

void
ObscuraDestroyScene(ObscuraScene **ptr, ObscuraAllocationCallbacks *allocator)
{
	if (*ptr != NULL) {
		for (uint32_t i = 0; i < (*ptr)->cameras_count; i++) {
			ObscuraComponent *component = (*ptr)->cameras[i];
			ObscuraDestroyComponent(&component, allocator);
		}
		allocator->free((*ptr)->cameras);

		for (uint32_t i = 0; i < (*ptr)->bounding_volumes_count; i++) {
			ObscuraComponent *component = (*ptr)->bounding_volumes[i];
			ObscuraDestroyComponent(&component, allocator);
		}
		allocator->free((*ptr)->bounding_volumes);

		for (uint32_t i = 0; i < (*ptr)->geometries_count; i++) {
			ObscuraComponent *component = (*ptr)->geometries[i];
			ObscuraDestroyComponent(&component, allocator);
		}
		allocator->free((*ptr)->geometries);

		for (uint32_t i = 0; i < (*ptr)->lights_count; i++) {
			ObscuraComponent *component = (*ptr)->lights[i];
			ObscuraDestroyComponent(&component, allocator);
		}
		allocator->free((*ptr)->lights);

		for (uint32_t i = 0; i < (*ptr)->materials_count; i++) {
			ObscuraComponent *component = (*ptr)->materials[i];
			ObscuraDestroyComponent(&component, allocator);
		}
		allocator->free((*ptr)->materials);

		for (uint32_t i = 0; i < (*ptr)->nodes_count; i++) {
			ObscuraNode *node = (*ptr)->nodes[i];
			ObscuraDestroyNode(&node, allocator);
		}
		allocator->free((*ptr)->nodes);

		allocator->free(*ptr);

		*ptr = NULL;
	}
}

ObscuraComponent *
ObscuraAcquireComponent(ObscuraScene *scene, ObscuraComponentFamily family, ObscuraAllocationCallbacks *allocator)
{
	ObscuraComponent *component = ObscuraCreateComponent(family, allocator);

	switch (family) {
	case OBSCURA_COMPONENT_FAMILY_CAMERA:
		if (scene->cameras_count < scene->cameras_capacity) {
			scene->cameras[scene->cameras_count] = component;
			scene->cameras_count++;
		}
		break;
	case OBSCURA_COMPONENT_FAMILY_BOUNDING_VOLUME:
		if (scene->bounding_volumes_count < scene->bounding_volumes_capacity) {
			scene->bounding_volumes[scene->bounding_volumes_count] = component;
			scene->bounding_volumes_count++;
		}
		break;
	case OBSCURA_COMPONENT_FAMILY_GEOMETRY:
		if (scene->geometries_count < scene->geometries_capacity) {
			scene->geometries[scene->geometries_count] = component;
			scene->geometries_count++;
		}
		break;
	case OBSCURA_COMPONENT_FAMILY_LIGHT:
		if (scene->lights_count < scene->lights_capacity) {
			scene->lights[scene->lights_count] = component;
			scene->lights_count++;
		}
		break;
	case OBSCURA_COMPONENT_FAMILY_MATERIAL:
		if (scene->materials_count < scene->materials_capacity) {
			scene->materials[scene->materials_count] = component;
			scene->materials_count++;
		}
		break;
	default:
		assert(false);
		break;
	}

	return component;
}

void
ObscuraReleaseComponent(ObscuraScene *scene, ObscuraComponent **ptr, ObscuraAllocationCallbacks *allocator)
{
	int family = (*ptr)->family;

	switch (family) {
	case OBSCURA_COMPONENT_FAMILY_CAMERA:
		for (uint32_t i = 0; i < scene->cameras_count; i++) {
			if (scene->cameras[i] == *ptr) {
				ObscuraDestroyComponent(ptr, allocator);
				scene->cameras[i] = scene->cameras[scene->cameras_count - 1];
				scene->cameras_count--;
			}
		}
		break;
	case OBSCURA_COMPONENT_FAMILY_BOUNDING_VOLUME:
		for (uint32_t i = 0; i < scene->bounding_volumes_count; i++) {
			if (scene->bounding_volumes[i] == *ptr) {
				ObscuraDestroyComponent(ptr, allocator);
				scene->bounding_volumes[i] = scene->bounding_volumes[scene->bounding_volumes_count - 1];
				scene->bounding_volumes_count--;
			}
		}
		break;
	case OBSCURA_COMPONENT_FAMILY_GEOMETRY:
		for (uint32_t i = 0; i < scene->geometries_count; i++) {
			if (scene->geometries[i] == *ptr) {
				ObscuraDestroyComponent(ptr, allocator);
				scene->geometries[i] = scene->geometries[scene->geometries_count - 1];
				scene->geometries_count--;
			}
		}
		break;
	case OBSCURA_COMPONENT_FAMILY_LIGHT:
		for (uint32_t i = 0; i < scene->lights_count; i++) {
			if (scene->lights[i] == *ptr) {
				ObscuraDestroyComponent(ptr, allocator);
				scene->lights[i] = scene->lights[scene->lights_count - 1];
				scene->lights_count--;
			}
		}
		break;
	case OBSCURA_COMPONENT_FAMILY_MATERIAL:
		for (uint32_t i = 0; i < scene->materials_count; i++) {
			if (scene->materials[i] == *ptr) {
				ObscuraDestroyComponent(ptr, allocator);
				scene->materials[i] = scene->materials[scene->materials_count - 1];
				scene->materials_count--;
			}
		}
		break;
	default:
		assert(false);
		break;
	}
}

ObscuraNode *
ObscuraAcquireNode(ObscuraScene *scene, ObscuraAllocationCallbacks *allocator)
{
	ObscuraNode *node = NULL;

	if (scene->nodes_count < scene->nodes_capacity) {
		node = ObscuraCreateNode(allocator);
		scene->nodes[scene->nodes_count] = node;
		scene->nodes_count++;
	}

	return node;
}

void
ObscuraReleaseNode(ObscuraScene *scene, ObscuraNode **ptr, ObscuraAllocationCallbacks *allocator)
{
	for (uint32_t i = 0; i < scene->nodes_count; i++) {
		if (scene->nodes[i] == *ptr) {
			ObscuraDestroyNode(ptr, allocator);
			scene->nodes[i] = scene->nodes[scene->nodes_count - 1];
			scene->nodes_count--;
		}
	}
}

void
ObscuraTraverseScene(ObscuraScene *scene, PFN_ObscuraSceneVisitorFunction visitor, void *arg,
	ObscuraAllocationCallbacks *allocator)
{
	for (uint32_t i = 0; i < scene->nodes_count; i++) {
		ObscuraNode *node = scene->nodes[i];
		traverse(node, visitor, arg, allocator);
	}
}
