#ifndef __OBSCURA_GEOMETRY_H__
#define __OBSCURA_GEOMETRY_H__ 1

#include "material.h"

typedef struct ObscuraGeometry {
	enum {
		OBSCURA_GEOMETRY_TYPE_SPHERE,
	}		 type;
	void		*geometry;
	ObscuraMaterial	*material;
} ObscuraGeometry;

typedef struct ObscuraGeometrySphere {
} ObscuraGeometrySphere;

#endif
