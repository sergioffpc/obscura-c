#ifndef __OBSCURA_SCENE_H__
#define __OBSCURA_SCENE_H__ 1

#include <stdint.h>

#include "math.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ObscuraComponentFamily {
	OBSCURA_COMPONENT_FAMILY_CAMERA,
	OBSCURA_COMPONENT_FAMILY_BOUNDING_VOLUME,
	OBSCURA_COMPONENT_FAMILY_GEOMETRY,
	OBSCURA_COMPONENT_FAMILY_LIGHT,
	OBSCURA_COMPONENT_FAMILY_MATERIAL,
} ObscuraComponentFamily;

typedef struct ObscuraComponent {
	ObscuraComponentFamily	 family;
	void			*component;
} ObscuraComponent;

extern ObscuraComponent *	ObscuraCreateComponent	(ObscuraComponentFamily, ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyComponent	(ObscuraComponent **, ObscuraAllocationCallbacks *);

typedef struct ObscuraNode {
	vec4	position;
	vec4	interest;
	vec4	up;

	uint32_t		  components_capacity;
	uint32_t		  components_count;
	ObscuraComponent	**components;

	uint32_t		  children_capacity;
	uint32_t		  children_count;
	struct ObscuraNode **children;
} ObscuraNode;

extern ObscuraNode *	ObscuraCreateNode	(ObscuraAllocationCallbacks *);
extern void		ObscuraDestroyNode	(ObscuraNode **, ObscuraAllocationCallbacks *);

extern ObscuraNode *	ObscuraAttachComponent	(ObscuraNode *, ObscuraComponent *);
extern void		ObscuraDetachComponent	(ObscuraNode *, ObscuraComponent *);

extern ObscuraComponent *	ObscuraFindComponent	(ObscuraNode *, int, int);
extern ObscuraComponent *	ObscuraFindAnyComponent	(ObscuraNode *, int);

extern ObscuraNode *	ObscuraAttachChild	(ObscuraNode *, ObscuraNode *);
extern void		ObscuraDetachChild	(ObscuraNode *, ObscuraNode *);

typedef struct ObscuraScene {
	uint32_t		  cameras_capacity;
	uint32_t		  cameras_count;
	ObscuraComponent	**cameras;

	uint32_t		  bounding_volumes_capacity;
	uint32_t		  bounding_volumes_count;
	ObscuraComponent	**bounding_volumes;

	uint32_t		  geometries_capacity;
	uint32_t		  geometries_count;
	ObscuraComponent	**geometries;

	uint32_t		  lights_capacity;
	uint32_t		  lights_count;
	ObscuraComponent	**lights;

	uint32_t		  materials_capacity;
	uint32_t		  materials_count;
	ObscuraComponent	**materials;

	uint32_t	  nodes_capacity;
	uint32_t	  nodes_count;
	ObscuraNode	**nodes;

	ObscuraNode	*view;
} ObscuraScene;

extern ObscuraScene *	ObscuraCreateScene	(ObscuraAllocationCallbacks *);
extern void		ObscuraDestroyScene	(ObscuraScene **, ObscuraAllocationCallbacks *);

extern ObscuraComponent *	ObscuraAcquireComponent	(ObscuraScene *, ObscuraComponentFamily, ObscuraAllocationCallbacks *);
extern void			ObscuraReleaseComponent	(ObscuraScene *, ObscuraComponent **, ObscuraAllocationCallbacks *);

extern ObscuraNode *	ObscuraAcquireNode	(ObscuraScene *, ObscuraAllocationCallbacks *);
extern void		ObscuraReleaseNode	(ObscuraScene *, ObscuraNode **, ObscuraAllocationCallbacks *);

typedef void	(*PFN_ObscuraSceneVisitorFunction)	(ObscuraNode *, void *);

extern void	ObscuraTraverseScene	(ObscuraScene *, PFN_ObscuraSceneVisitorFunction, void *);

#ifdef __cplusplus
}
#endif

#endif
