// SPDX-License-Identifier: MIT
/*
 * Copyright © 2016-2019 Intel Corporation
 */

#include <linux/types.h>

#include "gt/intel_gt.h"
#include "intel_guc_reg.h"
#include "intel_huc.h"
#include "intel_huc_print.h"
#include "i915_drv.h"
#include "i915_reg.h"
#include "pxp/intel_pxp_cmd_interface_43.h"

/**
 * DOC: HuC
 *
 * The HuC is a dedicated microcontroller for usage in media HEVC (High
 * Efficiency Video Coding) operations. Userspace can directly use the firmware
 * capabilities by adding HuC specific commands to batch buffers.
 *
 * The kernel driver is only responsible for loading the HuC firmware and
 * triggering its security authentication. This is done differently depending
 * on the platform:
 *
 * - older platforms (from Gen9 to most Gen12s): the load is performed via DMA
 *   and the authentication via GuC
 * - DG2: load and authentication are both performed via GSC.
 * - MTL and newer platforms: the load is performed via DMA (same as with
 *   not-DG2 older platforms), while the authentication is done in 2-steps,
 *   a first auth for clear-media workloads via GuC and a second one for all
 *   workloads via GSC.
 *
 * On platforms where the GuC does the authentication, to correctly do so the
 * HuC binary must be loaded before the GuC one.
 * Loading the HuC is optional; however, not using the HuC might negatively
 * impact power usage and/or performance of media workloads, depending on the
 * use-cases.
 * HuC must be reloaded on events that cause the WOPCM to lose its contents
 * (S3/S4, FLR); on older platforms the HuC must also be reloaded on GuC/GT
 * reset, while on newer ones it will survive that.
 *
 * See https://github.com/intel/media-driver for the latest details on HuC
 * functionality.
 */

/**
 * DOC: HuC Memory Management
 *
 * Similarly to the GuC, the HuC can't do any memory allocations on its own,
 * with the difference being that the allocations for HuC usage are handled by
 * the userspace driver instead of the kernel one. The HuC accesses the memory
 * via the PPGTT belonging to the context loaded on the VCS executing the
 * HuC-specific commands.
 */


static bool vcs_supported(struct intel_gt *gt)
{
	intel_engine_mask_t mask = gt->info.engine_mask;

	/*
	 * We reach here from i915_driver_early_probe for the primary GT before
	 * its engine mask is set, so we use the device info engine mask for it;
	 * this means we're not taking VCS fusing into account, but if the
	 * primary GT supports VCS engines we expect at least one of them to
	 * remain unfused so we're fine.
	 * For other GTs we expect the GT-specific mask to be set before we
	 * call this function.
	 */
	GEM_BUG_ON(!gt_is_root(gt) && !gt->info.engine_mask);

	if (gt_is_root(gt))
		mask = RUNTIME_INFO(gt->i915)->platform_engine_mask;
	else
		mask = gt->info.engine_mask;

	return __ENGINE_INSTANCES_MASK(mask, VCS0, I915_MAX_VCS);
}

void intel_huc_init_early(struct intel_huc *huc)
{
	struct drm_i915_private *i915 = huc_to_gt(huc)->i915;
	struct intel_gt *gt = huc_to_gt(huc);

	intel_uc_fw_init_early(&huc->fw, INTEL_UC_FW_TYPE_HUC, true);

	if (!vcs_supported(gt)) {
		intel_uc_fw_change_status(&huc->fw, INTEL_UC_FIRMWARE_NOT_SUPPORTED);
		return;
	}

	if (GRAPHICS_VER(i915) >= 11) {
		huc->status[INTEL_HUC_AUTH_BY_GUC].reg = GEN11_HUC_KERNEL_LOAD_INFO;
		huc->status[INTEL_HUC_AUTH_BY_GUC].mask = HUC_LOAD_SUCCESSFUL;
		huc->status[INTEL_HUC_AUTH_BY_GUC].value = HUC_LOAD_SUCCESSFUL;
	} else {
		huc->status[INTEL_HUC_AUTH_BY_GUC].reg = HUC_STATUS2;
		huc->status[INTEL_HUC_AUTH_BY_GUC].mask = HUC_FW_VERIFIED;
		huc->status[INTEL_HUC_AUTH_BY_GUC].value = HUC_FW_VERIFIED;
	}

	if (IS_DG2(i915)) {
		huc->status[INTEL_HUC_AUTH_BY_GSC].reg = GEN11_HUC_KERNEL_LOAD_INFO;
		huc->status[INTEL_HUC_AUTH_BY_GSC].mask = HUC_LOAD_SUCCESSFUL;
		huc->status[INTEL_HUC_AUTH_BY_GSC].value = HUC_LOAD_SUCCESSFUL;
	} else {
		huc->status[INTEL_HUC_AUTH_BY_GSC].reg = HECI_FWSTS(MTL_GSC_HECI1_BASE, 5);
		huc->status[INTEL_HUC_AUTH_BY_GSC].mask = HECI1_FWSTS5_HUC_AUTH_DONE;
		huc->status[INTEL_HUC_AUTH_BY_GSC].value = HECI1_FWSTS5_HUC_AUTH_DONE;
	}
}

#define HUC_LOAD_MODE_STRING(x) (x ? "GSC" : "legacy")
static int check_huc_loading_mode(struct intel_huc *huc)
{
	struct intel_gt *gt = huc_to_gt(huc);
	bool gsc_enabled = huc->fw.has_gsc_headers;

	/*
	 * The fuse for HuC load via GSC is only valid on platforms that have
	 * GuC deprivilege.
	 */
	if (HAS_GUC_DEPRIVILEGE(gt->i915))
		huc->loaded_via_gsc = intel_uncore_read(gt->uncore, GUC_SHIM_CONTROL2) &
				      GSC_LOADS_HUC;

	if (huc->loaded_via_gsc && !gsc_enabled) {
		huc_err(huc, "HW requires a GSC-enabled blob, but we found a legacy one\n");
		return -ENOEXEC;
	}

	/*
	 * On newer platforms we have GSC-enabled binaries but we load the HuC
	 * via DMA. To do so we need to find the location of the legacy-style
	 * binary inside the GSC-enabled one, which we do at fetch time. Make
	 * sure that we were able to do so if the fuse says we need to load via
	 * DMA and the binary is GSC-enabled.
	 */
	if (!huc->loaded_via_gsc && gsc_enabled && !huc->fw.dma_start_offset) {
		huc_err(huc, "HW in DMA mode, but we have an incompatible GSC-enabled blob\n");
		return -ENOEXEC;
	}

	/*
	 * If the HuC is loaded via GSC, we need to be able to access the GSC.
	 * On DG2 this is done via the mei components, while on newer platforms
	 * it is done via the GSCCS,
	 */
	if (huc->loaded_via_gsc) {
		if (IS_DG2(gt->i915)) {
			if (!IS_ENABLED(CONFIG_INTEL_MEI_PXP) ||
			    !IS_ENABLED(CONFIG_INTEL_MEI_GSC)) {
				huc_info(huc, "can't load due to missing mei modules\n");
				return -EIO;
			}
		} else {
			if (!HAS_ENGINE(gt, GSC0)) {
				huc_info(huc, "can't load due to missing GSCCS\n");
				return -EIO;
			}
		}
	}

	huc_dbg(huc, "loaded by GSC = %s\n", str_yes_no(huc->loaded_via_gsc));

	return 0;
}

int intel_huc_init(struct intel_huc *huc)
{
	struct intel_gt *gt = huc_to_gt(huc);
	int err;

	err = check_huc_loading_mode(huc);
	if (err)
		goto out;

	if (HAS_ENGINE(gt, GSC0)) {
		struct i915_vma *vma;

		vma = intel_guc_allocate_vma(&gt->uc.guc, PXP43_HUC_AUTH_INOUT_SIZE * 2);
		if (IS_ERR(vma)) {
			err = PTR_ERR(vma);
			huc_info(huc, "Failed to allocate heci pkt\n");
			goto out;
		}

		huc->heci_pkt = vma;
	}

	err = intel_uc_fw_init(&huc->fw);
	if (err)
		goto out_pkt;

	intel_uc_fw_change_status(&huc->fw, INTEL_UC_FIRMWARE_LOADABLE);

	return 0;

out_pkt:
	if (huc->heci_pkt)
		i915_vma_unpin_and_release(&huc->heci_pkt, 0);
out:
	intel_uc_fw_change_status(&huc->fw, INTEL_UC_FIRMWARE_INIT_FAIL);
	huc_info(huc, "initialization failed %pe\n", ERR_PTR(err));
	return err;
}

void intel_huc_fini(struct intel_huc *huc)
{
	if (huc->heci_pkt)
		i915_vma_unpin_and_release(&huc->heci_pkt, 0);

	if (intel_uc_fw_is_loadable(&huc->fw))
		intel_uc_fw_fini(&huc->fw);
}

static const char *auth_mode_string(struct intel_huc *huc,
				    enum intel_huc_authentication_type type)
{
	bool partial = huc->fw.has_gsc_headers && type == INTEL_HUC_AUTH_BY_GUC;

	return partial ? "clear media" : "all workloads";
}

int intel_huc_wait_for_auth_complete(struct intel_huc *huc,
				     enum intel_huc_authentication_type type)
{
	struct intel_gt *gt = huc_to_gt(huc);
	int ret;

	ret = __intel_wait_for_register(gt->uncore,
					huc->status[type].reg,
					huc->status[type].mask,
					huc->status[type].value,
					2, 50, NULL);

	if (ret) {
		huc_err(huc, "firmware not verified for %s: %pe\n",
			auth_mode_string(huc, type), ERR_PTR(ret));
		intel_uc_fw_change_status(&huc->fw, INTEL_UC_FIRMWARE_LOAD_FAIL);
		return ret;
	}

	intel_uc_fw_change_status(&huc->fw, INTEL_UC_FIRMWARE_RUNNING);
	huc_info(huc, "authenticated for %s\n", auth_mode_string(huc, type));
	return 0;
}

/**
 * intel_huc_auth() - Authenticate HuC uCode
 * @huc: intel_huc structure
 * @type: authentication type (via GuC or via GSC)
 *
 * Called after HuC and GuC firmware loading during intel_uc_init_hw().
 *
 * This function invokes the GuC action to authenticate the HuC firmware,
 * passing the offset of the RSA signature to intel_guc_auth_huc(). It then
 * waits for up to 50ms for firmware verification ACK.
 */
int intel_huc_auth(struct intel_huc *huc, enum intel_huc_authentication_type type)
{
	struct intel_gt *gt = huc_to_gt(huc);
	struct intel_guc *guc = &gt->uc.guc;
	int ret;

	if (!intel_uc_fw_is_loaded(&huc->fw))
		return -ENOEXEC;

	/* GSC will do the auth with the load */
	if (intel_huc_is_loaded_by_gsc(huc))
		return -ENODEV;

	if (intel_huc_is_authenticated(huc, type))
		return -EEXIST;

	ret = i915_inject_probe_error(gt->i915, -ENXIO);
	if (ret)
		goto fail;

	switch (type) {
	case INTEL_HUC_AUTH_BY_GUC:
		ret = intel_guc_auth_huc(guc, intel_guc_ggtt_offset(guc, huc->fw.rsa_data));
		break;
	case INTEL_HUC_AUTH_BY_GSC:
		ret = intel_huc_fw_auth_via_gsccs(huc);
		break;
	default:
		MISSING_CASE(type);
		ret = -EINVAL;
	}
	if (ret)
		goto fail;

	/* Check authentication status, it should be done by now */
	ret = intel_huc_wait_for_auth_complete(huc, type);
	if (ret)
		goto fail;

	return 0;

fail:
	huc_probe_error(huc, "%s authentication failed %pe\n",
			auth_mode_string(huc, type), ERR_PTR(ret));
	return ret;
}

bool intel_huc_is_authenticated(struct intel_huc *huc,
				enum intel_huc_authentication_type type)
{
	struct intel_gt *gt = huc_to_gt(huc);
	intel_wakeref_t wakeref;
	u32 status = 0;

	with_intel_runtime_pm(gt->uncore->rpm, wakeref)
		status = intel_uncore_read(gt->uncore, huc->status[type].reg);

	return (status & huc->status[type].mask) == huc->status[type].value;
}

static bool huc_is_fully_authenticated(struct intel_huc *huc)
{
	struct intel_uc_fw *huc_fw = &huc->fw;

	/*
	 * in the non-POR MTL flow, the GSC re-uses the same regs as GuC (like
	 * on DG2). This check can be dropped once the new IFWI which supports
	 * the POR flow has been propagated to all users.
	 */
	if (IS_METEORLAKE(huc_to_gt(huc)->i915) && huc->loaded_via_gsc)
		return intel_huc_is_authenticated(huc, INTEL_HUC_AUTH_BY_GUC);

	if (!huc_fw->has_gsc_headers)
		return intel_huc_is_authenticated(huc, INTEL_HUC_AUTH_BY_GUC);
	else if (intel_huc_is_loaded_by_gsc(huc) || HAS_ENGINE(huc_to_gt(huc), GSC0))
		return intel_huc_is_authenticated(huc, INTEL_HUC_AUTH_BY_GSC);
	else
		return false;
}

bool intel_huc_is_fully_authenticated(struct intel_huc *huc)
{
	return huc_is_fully_authenticated(huc);
}
/**
 * intel_huc_check_status() - check HuC status
 * @huc: intel_huc structure
 *
 * This function reads status register to verify if HuC
 * firmware was successfully loaded.
 *
 * The return values match what is expected for the I915_PARAM_HUC_STATUS
 * getparam.
 */
int intel_huc_check_status(struct intel_huc *huc)
{
	struct intel_uc_fw *huc_fw = &huc->fw;

	switch (__intel_uc_fw_status(huc_fw)) {
	case INTEL_UC_FIRMWARE_NOT_SUPPORTED:
		return -ENODEV;
	case INTEL_UC_FIRMWARE_DISABLED:
		return -EOPNOTSUPP;
	case INTEL_UC_FIRMWARE_MISSING:
		return -ENOPKG;
	case INTEL_UC_FIRMWARE_ERROR:
		return -ENOEXEC;
	case INTEL_UC_FIRMWARE_INIT_FAIL:
		return -ENOMEM;
	case INTEL_UC_FIRMWARE_LOAD_FAIL:
		return -EIO;
	default:
		break;
	}

	/*
	 * GSC-enabled binaries loaded via DMA are first partially
	 * authenticated by GuC and then fully authenticated by GSC
	 */
	if (huc_is_fully_authenticated(huc))
		return 1; /* full auth */
	else if (huc_fw->has_gsc_headers && !intel_huc_is_loaded_by_gsc(huc) &&
		 intel_huc_is_authenticated(huc, INTEL_HUC_AUTH_BY_GUC))
		return 2; /* clear media only */
	else
		return 0;
}

void intel_huc_update_auth_status(struct intel_huc *huc)
{
	if (!intel_uc_fw_is_loadable(&huc->fw))
		return;

	if (!huc->fw.has_gsc_headers)
		return;

	if (huc_is_fully_authenticated(huc))
		intel_uc_fw_change_status(&huc->fw,
					  INTEL_UC_FIRMWARE_RUNNING);
}

/**
 * intel_huc_load_status - dump information about HuC load status
 * @huc: the HuC
 * @p: the &drm_printer
 *
 * Pretty printer for HuC load status.
 */
void intel_huc_load_status(struct intel_huc *huc, struct drm_printer *p)
{
	struct intel_gt *gt = huc_to_gt(huc);
	intel_wakeref_t wakeref;

	if (!intel_huc_is_supported(huc)) {
		drm_printf(p, "HuC not supported\n");
		return;
	}

	if (!intel_huc_is_wanted(huc)) {
		drm_printf(p, "HuC disabled\n");
		return;
	}

	intel_uc_fw_dump(&huc->fw, p);

	with_intel_runtime_pm(gt->uncore->rpm, wakeref)
		drm_printf(p, "HuC status: 0x%08x\n",
			   intel_uncore_read(gt->uncore, huc->status[INTEL_HUC_AUTH_BY_GUC].reg));
}
