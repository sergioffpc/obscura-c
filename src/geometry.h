#ifndef __OBSCURA_GEOMETRY_H__
#define __OBSCURA_GEOMETRY_H__ 1

#include "material.h"
#include "memory.h"

typedef enum ObscuraGeometryType {
	OBSCURA_GEOMETRY_TYPE_PARAMETRIC_SPHERE,
} ObscuraGeometryType;

typedef struct ObscuraGeometry {
	ObscuraGeometryType	 type;
	void			*geometry;

	ObscuraMaterial		*material;
} ObscuraGeometry;

extern ObscuraGeometry *	ObscuraCreateGeometry(ObscuraGeometryType, ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyGeometry(ObscuraGeometry **, ObscuraAllocationCallbacks *);

typedef struct ObscuraGeometrySphere {
	float	radius;
} ObscuraGeometrySphere;

#endif
