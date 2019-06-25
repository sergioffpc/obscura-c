#ifndef __OBSCURA_SCENE_H__
#define __OBSCURA_SCENE_H__ 1

#include <stdint.h>

#include "memory.h"
#include "transform.h"

typedef enum ObscuraSceneComponentType {
	OBSCURA_SCENE_COMPONENT_TYPE_CAMERA_PERSPECTIVE,
	OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE_FRUSTUM,
} ObscuraSceneComponentType;

typedef struct ObscuraSceneComponent {
	ObscuraSceneComponentType	 type;
	void				*component;
} ObscuraSceneComponent;

extern ObscuraSceneComponent *	ObscuraCreateComponent	(ObscuraSceneComponentType, ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyComponent	(ObscuraSceneComponent **, ObscuraAllocationCallbacks *);

typedef struct ObscuraSceneNode {
	vec4	position;
	vec4	interest;
	vec4	up;

	uint32_t		  components_count;
	ObscuraSceneComponent	**components;

	uint32_t		  children_count;
	struct ObscuraSceneNode **children;
} ObscuraSceneNode;

extern ObscuraSceneNode *	ObscuraCreateNode	(ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyNode	(ObscuraSceneNode **, ObscuraAllocationCallbacks *);

extern ObscuraSceneNode *	ObscuraAddChild	(ObscuraSceneNode *, ObscuraAllocationCallbacks *);

extern ObscuraSceneComponent *	ObscuraAddComponent	(ObscuraSceneNode *,
							 ObscuraSceneComponentType,
							 ObscuraAllocationCallbacks *);
extern ObscuraSceneComponent *	ObscuraFindComponent	(ObscuraSceneNode *, ObscuraSceneComponentType);

typedef struct ObscuraScene {
	ObscuraSceneNode	*view;

	uint32_t		  nodes_count;
	ObscuraSceneNode	**nodes;
} ObscuraScene;

extern ObscuraScene *	ObscuraCreateScene	(ObscuraAllocationCallbacks *);
extern void		ObscuraDestroyScene	(ObscuraScene **, ObscuraAllocationCallbacks *);

extern ObscuraSceneNode *	ObscuraAddNode	(ObscuraScene *, ObscuraAllocationCallbacks *);

#endif
