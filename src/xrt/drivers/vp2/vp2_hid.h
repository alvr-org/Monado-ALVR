// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Implementation of the Vive Pro 2 HID interface.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_vp2
 */

#pragma once

#include "xrt/xrt_defines.h"

#include "os/os_hid.h"

#include <assert.h>


#ifdef __cplusplus
extern "C" {
#endif


#define VP2_VID 0x0bb4
#define VP2_PID 0x0342

enum vp2_resolution
{
	VP2_RESOLUTION_2448_1224_90_03 = 0,
	VP2_RESOLUTION_2448_1224_120_05 = 1,
	VP2_RESOLUTION_3264_1632_90_00 = 2,
	VP2_RESOLUTION_3680_1836_90_02 = 3,
	VP2_RESOLUTION_4896_2448_90_02 = 4,
	VP2_RESOLUTION_4896_2448_120_02 = 5,
};

static inline void
vp2_resolution_get_extents(enum vp2_resolution res, int *out_w, int *out_h)
{
	switch (res) {
	case VP2_RESOLUTION_2448_1224_90_03:
	case VP2_RESOLUTION_2448_1224_120_05:
		*out_w = 2448;
		*out_h = 1224;
		break;
	case VP2_RESOLUTION_3264_1632_90_00:
		*out_w = 3264;
		*out_h = 1632;
		break;
	case VP2_RESOLUTION_3680_1836_90_02:
		*out_w = 3680;
		*out_h = 1836;
		break;
	case VP2_RESOLUTION_4896_2448_90_02:
	case VP2_RESOLUTION_4896_2448_120_02:
		*out_w = 4896;
		*out_h = 2448;
		break;
	default:
		assert(!"unreachable: bad resolution");
		*out_w = 0;
		*out_h = 0;
		break;
	}
}

static inline double
vp2_resolution_get_refresh_rate(enum vp2_resolution res)
{
	switch (res) {
	case VP2_RESOLUTION_2448_1224_90_03: return 90.03;
	case VP2_RESOLUTION_2448_1224_120_05: return 120.05;
	case VP2_RESOLUTION_3264_1632_90_00: return 90.0;
	case VP2_RESOLUTION_3680_1836_90_02:
	case VP2_RESOLUTION_4896_2448_90_02: return 90.02;
	case VP2_RESOLUTION_4896_2448_120_02: return 120.02;
	default: assert(!"unreachable: bad resolution"); return 0.0;
	}
}

struct vp2_hid;

int
vp2_hid_open(struct os_hid_device *hid_dev, struct vp2_hid **out_hid);

enum vp2_resolution
vp2_get_resolution(struct vp2_hid *vp2);

struct vp2_config *
vp2_get_config(struct vp2_hid *vpd);

void
vp2_hid_destroy(struct vp2_hid *vp2);

const char *
vp2_get_serial(struct vp2_hid *vp2);

int
vp2_set_noise_cancelling(struct vp2_hid *vp2, bool enabled);

float
vp2_get_brightness(struct vp2_hid *vp2);

int
vp2_set_brightness(struct vp2_hid *vp2, float brightness);

/*!
 * Prepares the display panels to have video terminated.
 */
int
vp2_prepare_shutdown(struct vp2_hid *vp2);

bool
vp2_get_status(struct vp2_hid *vp2,
               bool *out_proximity,
               float *out_lens_separation_meters,
               bool *out_system,
               bool *out_volume_up,
               bool *out_volume_down,
               bool *out_mute_mic);


#ifdef __cplusplus
} // extern "C"
#endif
