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

#ifndef _BACKPORT_BACKPORT_H
#define _BACKPORT_BACKPORT_H
//#include <generated/autoconf.h>
#include <linux/kconfig.h>
#include <backport/autoconf.h>

#ifndef __ASSEMBLY__
#define LINUX_I915_BACKPORT(__sym) i915bkpt_ ##__sym
#define LINUX_DMABUF_BACKPORT(__sym) dmabufbkpt_ ##__sym
#endif

#ifndef CONFIG_BACKPORT_SYNC
#define CONFIG_BACKPORT_SYNC 1
#endif

#ifndef CPTCFG_DRM_FBDEV_EMULATION
#define CPTCFG_DRM_FBDEV_EMULATION 1
#endif

#ifndef CPTCFG_AGP
//#define CPTCFG_AGP 1
#endif

#ifndef CPTCFG_DRM_I915_FBDEV
#define CPTCFG_DRM_I915_FBDEV 1
#endif

#ifndef CPTCFG_DRM_MIPI_DSI
#define CPTCFG_DRM_MIPI_DSI 1
#endif

#ifndef CPTCFG_DRM_I915_KMS
#define CPTCFG_DRM_I915_KMS 1
#endif

#ifndef CPTCFG_DRM_I915_CAPTURE_ERROR
#define CPTCFG_DRM_I915_CAPTURE_ERROR 1
#endif

#endif