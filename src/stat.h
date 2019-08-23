#ifndef __OBSCURA_STAT_H__
#define __OBSCURA_STAT_H__ 1

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ObscuraCounterType {
	OBSCURA_COUNTER_TYPE_CAMERA,
	OBSCURA_COUNTER_TYPE_REFLECTION,
	OBSCURA_COUNTER_TYPE_REFRACTION,
	OBSCURA_COUNTER_TYPE_SHADOW,

	__COUNTER_TYPE_NUM_ELMS,
} ObscuraCounterType;

typedef struct ObscuraCounters {
	uint64_t	counter[__COUNTER_TYPE_NUM_ELMS];
} ObscuraCounters;

#ifdef __cplusplus
}
#endif

#endif
