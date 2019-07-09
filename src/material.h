#ifndef __OBSCURA_MATERIAL_H__
#define __OBSCURA_MATERIAL_H__ 1

#include <stdint.h>

#include "memory.h"
#include "tensor.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ObscuraMaterialType {
	OBSCURA_MATERIAL_TYPE_COLOR,
} ObscuraMaterialType;

typedef struct ObscuraMaterial {
	ObscuraMaterialType	 type;
	void			*material;
} ObscuraMaterial;

extern ObscuraMaterial *	ObscuraCreateMaterial(ObscuraAllocationCallbacks *);
extern void			ObscuraDestroyMaterial(ObscuraMaterial **, ObscuraAllocationCallbacks *);

extern ObscuraMaterial *	ObscuraBindMaterial(ObscuraMaterial *, ObscuraMaterialType, ObscuraAllocationCallbacks *);

typedef struct ObscuraMaterialColor {
	vec4	color;
} ObscuraMaterialColor;

#ifdef __cplusplus
}
#endif

#endif
