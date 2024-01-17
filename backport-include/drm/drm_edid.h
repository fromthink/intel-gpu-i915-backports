#ifndef _BACKPORT_DRM_EDID_H
#define _BACKPORT_DRM_EDID_H
#include_next <drm/drm_edid.h>

#ifdef DRM_EDID_IS_DIGITAL_NOT_PRESENT
#define drm_edid_is_digital LINUX_I915_BACKPORT(drm_edid_is_digital)
bool drm_edid_is_digital(const struct drm_edid *drm_edid);
#endif

#endif
