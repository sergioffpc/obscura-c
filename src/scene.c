#include "scene.h"

ObscuraSceneNode *ObscuraCreateNode(ObscuraAllocationCallbacks *allocator) {
	ObscuraSceneNode *node = allocator->allocation(sizeof(ObscuraSceneNode), 8);

	return node;
}

void ObscuraDestroyNode(ObscuraSceneNode **ptr, ObscuraAllocationCallbacks *allocator) {
	allocator->free(*ptr);

	*ptr = NULL;
}

ObscuraSceneComponent *ObscuraAddComponent(ObscuraSceneNode *node,
					   ObscuraSceneComponentType type,
					   ObscuraAllocationCallbacks *allocator) {
	return NULL;
}

void *ObscuraFindComponent(ObscuraSceneNode *node, ObscuraSceneComponentType type) {
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
	return NULL;
}
