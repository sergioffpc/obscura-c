#ifndef __OBSCURA_MATERIAL_H__
#define __OBSCURA_MATERIAL_H__ 1

#include <stdint.h>

#include "mathematics.h"
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

typedef struct ObscuraMaterialColor {
	vec4	color;
} ObscuraMaterialColor;

#define OBSCURA_COLOR2UINT32(c)	\
	(((uint32_t) (((int) ((c)[0] * 255) & 0xff) << 16) | (((int) ((c)[1] * 255) & 0xff) << 8) | ((int) ((c)[2] * 255) & 0xff)))

#endif
