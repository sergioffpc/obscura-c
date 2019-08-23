#ifndef __OBSCURA_STAT_H__
#define __OBSCURA_STAT_H__ 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ObscuraPerfCounterType {
	OBSCURA_COUNTER_TYPE_CAMERA,
	OBSCURA_COUNTER_TYPE_REFLECTION,
	OBSCURA_COUNTER_TYPE_REFRACTION,
	OBSCURA_COUNTER_TYPE_SHADOW,

	OBSCURA_COUNTER_TYPE_RAY_GEOM_INTERSECT,

	__COUNTER_TYPE_NUM_ELMS,
} ObscuraPerfCounterType;

typedef uint64_t	ObscuraPerfCounters[__COUNTER_TYPE_NUM_ELMS];

extern ObscuraPerfCounters	ObscuraCounters;

#ifdef __cplusplus
}
#endif

#endif
