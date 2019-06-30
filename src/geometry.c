#include "geometry.h"

ObscuraGeometry *ObscuraCreateGeometry(ObscuraGeometryType type, ObscuraAllocationCallbacks *allocator) {
	ObscuraGeometry *geometry = allocator->allocation(sizeof(ObscuraGeometry), 8);
	geometry->type = type;

	switch (geometry->type) {
	case OBSCURA_GEOMETRY_TYPE_PARAMETRIC_SPHERE:
		geometry->geometry = allocator->allocation(sizeof(ObscuraGeometrySphere), 8);
		break;
	}

	return geometry;
}

void ObscuraDestroyGeometry(ObscuraGeometry **ptr, ObscuraAllocationCallbacks *allocator) {
	allocator->free((*ptr)->geometry);
	allocator->free(*ptr);

	*ptr = NULL;
}
