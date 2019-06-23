#ifndef __OBSCURA_SCENE_H__
#define __OBSCURA_SCENE_H__ 1

#include <stdint.h>

#include "memory.h"
#include "transform.h"

typedef enum ObscuraSceneComponentType {
	OBSCURA_SCENE_COMPONENT_TYPE_CAMERA,
	OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE,
	OBSCURA_SCENE_COMPONENT_TYPE_GEOMETRY,
	OBSCURA_SCENE_COMPONENT_TYPE_LIGHT,
} ObscuraSceneComponentType;

typedef struct ObscuraSceneComponent {
	ObscuraSceneComponentType	 type;
	void				*component;
} ObscuraSceneComponent;

typedef struct ObscuraSceneNode {
	mat4	transformation;

	uint32_t		  components_count;
	ObscuraSceneComponent	**components;

	uint32_t		  children_count;
	struct ObscuraSceneNode **children;
} ObscuraSceneNode;

extern ObscuraSceneNode *	ObscuraCreateNode	(ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyNode	(ObscuraSceneNode **, ObscuraAllocationCallbacks *);

extern ObscuraSceneComponent *	ObscuraAddComponent	(ObscuraSceneNode *,
							 ObscuraSceneComponentType,
							 ObscuraAllocationCallbacks *);
extern void *	ObscuraFindComponent	(ObscuraSceneNode *, ObscuraSceneComponentType);

typedef struct ObscuraScene {
	ObscuraSceneNode	*view;

	uint32_t		  nodes_count;
	ObscuraSceneNode	**nodes;
} ObscuraScene;

extern ObscuraScene *	ObscuraCreateScene	(ObscuraAllocationCallbacks *);
extern void		ObscuraDestroyScene	(ObscuraScene **, ObscuraAllocationCallbacks *);

extern ObscuraSceneNode *	ObscuraAddNode	(ObscuraScene *, ObscuraAllocationCallbacks *);

#endif
