#ifndef __OBSCURA_CAMERA_H__
#define __OBSCURA_CAMERA_H__ 1

#include <stdint.h>

#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ObscuraCameraProjectionType {
	OBSCURA_CAMERA_PROJECTION_TYPE_PERSPECTIVE,
} ObscuraCameraProjectionType;

typedef enum ObscuraCameraFilterType {
	OBSCURA_CAMERA_FILTER_TYPE_COLOR,
	OBSCURA_CAMERA_FILTER_TYPE_DEPTH,
	OBSCURA_CAMERA_FILTER_TYPE_NORMAL,
} ObscuraCameraFilterType;

typedef enum ObscuraCameraAntiAliasingTechnique {
	OBSCURA_CAMERA_ANTI_ALIASING_TECHNIQUE_NONE,

	OBSCURA_CAMERA_ANTI_ALIASING_TECHNIQUE_SSAA_STOCHASTIC,
} ObscuraCameraAntiAliasingTechnique;

typedef struct ObscuraCamera {
	ObscuraCameraProjectionType	 type;
	void				*projection;

	ObscuraCameraFilterType	filter;

	ObscuraCameraAntiAliasingTechnique	anti_aliasing;
	uint32_t				samples_count;
} ObscuraCamera;

extern ObscuraCamera *	ObscuraCreateCamera	(ObscuraAllocationCallbacks *);
extern void		ObscuraDestroyCamera	(ObscuraCamera **, ObscuraAllocationCallbacks *);

extern ObscuraCamera *	ObscuraBindProjection	(ObscuraCamera *, ObscuraCameraProjectionType, ObscuraAllocationCallbacks *);

/*
 * Describes the field of view of a perspective camera.
 */
typedef struct ObscuraCameraPerspective {
	float	aspect_ratio;
	float	yfov;
	float	znear;
	float	zfar;
} ObscuraCameraPerspective;

#ifdef __cplusplus
}
#endif

#endif
