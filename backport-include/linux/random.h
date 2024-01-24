#ifndef _BACKPORT_RANDOM_H
#define _BACKPORT_RANDOM_H

#include_next <linux/random.h>
#include <linux/prandom.h>

#ifndef BPM_GET_RANDOM_U32_BELOW_AVAILABLE
#define get_random_u32_below prandom_u32_max
#endif

#endif
