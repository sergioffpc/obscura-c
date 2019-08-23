#ifndef __OBSCURA_SHADE_H__
#define __OBSCURA_SHADE_H__ 1

#include "scene.h"
#include "tensor.h"
#include "visibility.h"

#ifdef __cplusplus
extern "C" {
#endif

extern vec4	ObscuraShade	(ObscuraVisible *, ObscuraNode *, ObscuraNode *);

#ifdef __cplusplus
}
#endif

#endif
