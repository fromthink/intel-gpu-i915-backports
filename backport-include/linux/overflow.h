#ifndef __BP_OVERFLOW_H
#define __BP_OVERFLOW_H

#include_next <linux/overflow.h>

#ifndef BPM_OVERFLOWS_TYPE_AVAILABLE
/* Note we don't consider signbits :| */
#define overflows_type(x, T) (sizeof(x) > sizeof(T) && (x) >> BITS_PER_TYPE(T))

#define __overflows_type_constexpr(x, T) (			\
	is_unsigned_type(typeof(x)) ?				\
		(x) > type_max(typeof(T)) :			\
	is_unsigned_type(typeof(T)) ?				\
		(x) < 0 || (x) > type_max(typeof(T)) :		\
	(x) < type_min(typeof(T)) || (x) > type_max(typeof(T)))

/**
 * castable_to_type - like __same_type(), but also allows for casted literals
 *
 * @n: variable or constant value
 * @T: variable or data type
 *
 * Unlike the __same_type() macro, this allows a constant value as the
 * first argument. If this value would not overflow into an assignment
 * of the second argument's type, it returns true. Otherwise, this falls
 * back to __same_type().
 */
#define castable_to_type(n, T)						\
	__builtin_choose_expr(__is_constexpr(n),			\
			      !__overflows_type_constexpr(n, T),	\
			      __same_type(n, T))
#endif

#endif /* __BP_OVERFLOW_H */
