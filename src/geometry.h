#ifndef __OBSCURA_GEOMETRY_H__
#define __OBSCURA_GEOMETRY_H__ 1

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ObscuraGeometryType {
	OBSCURA_GEOMETRY_TYPE_PARAMETRIC_SPHERE,
} ObscuraGeometryType;

typedef struct ObscuraGeometry {
	ObscuraGeometryType	 type;
	void			*geometry;
} ObscuraGeometry;

extern ObscuraGeometry *	ObscuraCreateGeometry(ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyGeometry(ObscuraGeometry **, ObscuraAllocationCallbacks *);

extern ObscuraGeometry *	ObscuraBindGeometry(ObscuraGeometry *, ObscuraGeometryType, ObscuraAllocationCallbacks *);

typedef struct ObscuraGeometrySphere {
	float	radius;
} ObscuraGeometrySphere;

#ifdef __cplusplus
}
#endif

#endif
