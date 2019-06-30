#ifndef __OBSCURA_CAMERA_H__
#define __OBSCURA_CAMERA_H__ 1

#include "memory.h"

typedef enum ObscuraCameraProjectionType {
	OBSCURA_CAMERA_PROJECTION_TYPE_PERSPECTIVE,
} ObscuraCameraProjectionType;

typedef struct ObscuraCamera {
	ObscuraCameraProjectionType	 type;
	void				*projection;
} ObscuraCamera;

extern ObscuraCamera *	ObscuraCreateCamera	(ObscuraCameraProjectionType, ObscuraAllocationCallbacks *);
extern void		ObscuraDestroyCamera	(ObscuraCamera **, ObscuraAllocationCallbacks *);

/*
 * Describes the field of view of a perspective camera.
 */
typedef struct ObscuraCameraPerspective {
	float	aspect_ratio;
	float	yfov;
	float	znear;
	float	zfar;
} ObscuraCameraPerspective;

#endif
