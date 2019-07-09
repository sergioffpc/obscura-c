#include <assert.h>
#include <stdbool.h>

#include "geometry.h"

ObscuraGeometry *
ObscuraCreateGeometry(ObscuraAllocationCallbacks *allocator)
{
	ObscuraGeometry *geometry = allocator->allocation(sizeof(ObscuraGeometry), 8);

	return geometry;
}

void
ObscuraDestroyGeometry(ObscuraGeometry **ptr, ObscuraAllocationCallbacks *allocator)
{
	allocator->free((*ptr)->geometry);
	allocator->free(*ptr);

	*ptr = NULL;
}

ObscuraGeometry *
ObscuraBindGeometry(ObscuraGeometry *geometry, ObscuraGeometryType type, ObscuraAllocationCallbacks *allocator)
{
	geometry->type = type;

	switch (geometry->type) {
	case OBSCURA_GEOMETRY_TYPE_PARAMETRIC_SPHERE:
		geometry->geometry = allocator->allocation(sizeof(ObscuraGeometrySphere), 8);
		break;
	default:
		assert(false);
		break;
	}

	return geometry;
}
