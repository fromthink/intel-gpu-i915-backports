#ifndef __BACKPORT_LINUX_COMPILER_H
#define __BACKPORT_LINUX_COMPILER_H
#include_next <linux/compiler.h>

#ifndef BPM_OVERFLOWS_TYPE_AVAILABLE
#define is_unsigned_type(type) (!is_signed_type(type))
#endif

#endif
