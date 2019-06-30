#ifndef __OBSCURA_SCENE_H__
#define __OBSCURA_SCENE_H__ 1

#include <stdint.h>

#include "math.h"
#include "memory.h"

typedef enum ObscuraSceneComponentFamily {
	OBSCURA_SCENE_COMPONENT_FAMILY_CAMERA		= 0x00010000,
	OBSCURA_SCENE_COMPONENT_FAMILY_COLLIDABLE	= 0x00020000,
	OBSCURA_SCENE_COMPONENT_FAMILY_GEOMETRY		= 0x00040000,
	OBSCURA_SCENE_COMPONENT_FAMILY_MATERIAL		= 0x00080000,
} ObscuraSceneComponentFamily;

typedef enum ObscuraSceneComponentType {
	OBSCURA_SCENE_COMPONENT_TYPE_CAMERA_PERSPECTIVE	= OBSCURA_SCENE_COMPONENT_FAMILY_CAMERA | 0x0001,

	OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE_BALL	= OBSCURA_SCENE_COMPONENT_FAMILY_COLLIDABLE | 0x0002,
	OBSCURA_SCENE_COMPONENT_TYPE_COLLIDABLE_FRUSTUM	= OBSCURA_SCENE_COMPONENT_FAMILY_COLLIDABLE | 0x0004,

	OBSCURA_SCENE_COMPONENT_TYPE_GEOMETRY_SPHERE	= OBSCURA_SCENE_COMPONENT_FAMILY_GEOMETRY | 0x0008,

	OBSCURA_SCENE_COMPONENT_TYPE_MATERIAL_COLOR	= OBSCURA_SCENE_COMPONENT_FAMILY_MATERIAL | 0x0010,
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

	uint32_t		  components_capacity;
	uint32_t		  components_count;
	ObscuraSceneComponent	**components;

	uint32_t		  children_capacity;
	uint32_t		  children_count;
	struct ObscuraSceneNode **children;
} ObscuraSceneNode;

extern ObscuraSceneNode *	ObscuraCreateNode	(ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyNode	(ObscuraSceneNode **, ObscuraAllocationCallbacks *);

extern ObscuraSceneNode *	ObscuraAddChild	(ObscuraSceneNode *, ObscuraAllocationCallbacks *);

extern ObscuraSceneComponent *	ObscuraAddComponent	(ObscuraSceneNode *,
							 ObscuraSceneComponentType,
							 ObscuraAllocationCallbacks *);
extern ObscuraSceneComponent *	ObscuraFindComponent	(ObscuraSceneNode *, ObscuraSceneComponentFamily);

typedef struct ObscuraScene {
	ObscuraSceneNode	*view;

	uint32_t		  nodes_capacity;
	uint32_t		  nodes_count;
	ObscuraSceneNode	**nodes;
} ObscuraScene;

extern ObscuraScene *	ObscuraCreateScene	(ObscuraAllocationCallbacks *);
extern void		ObscuraDestroyScene	(ObscuraScene **, ObscuraAllocationCallbacks *);

extern ObscuraSceneNode *	ObscuraAddNode	(ObscuraScene *, ObscuraAllocationCallbacks *);

typedef void	(*PFN_ObscuraSceneVisitorFunction)	(ObscuraSceneNode *, void *);

extern void	ObscuraTraverseScene	(ObscuraScene *, PFN_ObscuraSceneVisitorFunction, void *);

#endif
