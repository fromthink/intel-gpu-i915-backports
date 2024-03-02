/* SPDX-License-Identifier: MIT */

#ifndef _BACKPORT_VIDEO_NOMODESET_H
#define _BACKPORT_VIDEO_NOMODESET_H

#ifdef BPM_VIDEO_FIRMWARE_DRIVERS_ONLY_NOT_EXPORTED
#include_next <video/nomodeset.h>
#elif defined(BPM_VGACON_TEXT_FORCE_NOT_PRESENT)
#define video_firmware_drivers_only drm_firmware_drivers_only
#include_next <drm/drm_drv.h>
#else
#define video_firmware_drivers_only vgacon_text_force
#include_next <linux/console.h>
#endif

#endif
