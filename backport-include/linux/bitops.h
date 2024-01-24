/*
 * Copyright © 2015 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 *
 */

#ifndef _BACKPORT_LINUX_BITOPS_H
#define _BACKPORT_LINUX_BITOPS_H
#include_next <linux/bitops.h>

#ifndef BITS_PER_TYPE
#define BITS_PER_TYPE(type) (sizeof(type) * BITS_PER_BYTE)
#endif
#ifndef BITS_TO_LONGS
#define BITS_TO_LONGS(nr)       DIV_ROUND_UP(nr, BITS_PER_TYPE(long))
#endif
#ifndef GENMASK
#define GENMASK(h, l) \
	(((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#endif

#ifdef BPM_FOR_EACH_CLEAR_BITRANGE_NOT_PRESENT
/**
 * for_each_clear_bitrange - iterate over all unset bit ranges [b; e)
 * @b: bit offset of start of current bitrange (first unset bit)
 * @e: bit offset of end of current bitrange (first set bit)
 * @addr: bitmap address to base the search on
 * @size: bitmap size in number of bits
 */
#define for_each_clear_bitrange(b, e, addr, size)		\
	for ((b) = 0;						\
	     (b) = find_next_zero_bit((addr), (size), (b)),	\
	     (e) = find_next_bit((addr), (size), (b) + 1),	\
	     (b) < (size);					\
	     (b) = (e) + 1)
#endif

#endif
