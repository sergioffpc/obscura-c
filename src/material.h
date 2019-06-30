#ifndef __OBSCURA_MATERIAL_H__
#define __OBSCURA_MATERIAL_H__ 1

#include <stdint.h>

#include "memory.h"

typedef enum ObscuraMaterialType {
	OBSCURA_MATERIAL_TYPE_COLOR,
} ObscuraMaterialType;

typedef struct ObscuraMaterial {
	ObscuraMaterialType	 type;
	void			*material;
} ObscuraMaterial;

extern ObscuraMaterial *	ObscuraCreateMaterial(ObscuraMaterialType, ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyMaterial(ObscuraMaterial **, ObscuraAllocationCallbacks *);

typedef union ObscuraMaterialColor {
	struct { float r, g, b, a; };
	uint32_t	color;
} ObscuraMaterialColor;

#endif
