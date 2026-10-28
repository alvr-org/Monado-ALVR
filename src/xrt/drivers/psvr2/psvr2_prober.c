// Copyright 2020-2021, Collabora, Ltd.
// Copyright 2023, Jan Schmidt
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  PSVR2 HMD device prober
 *
 * @author Jan Schmidt <jan@centricular.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @ingroup drv_psvr2
 */

#include "xrt/xrt_prober.h"

#include "util/u_misc.h"
#include "util/u_debug.h"

#include "psvr2_interface.h"


int
psvr2_found(struct xrt_prober *xp,
            struct xrt_prober_device **devices,
            size_t device_count,
            size_t index,
            cJSON *attached_data,
            struct xrt_device **out_xdevs)
{
	struct xrt_device *hmd = psvr2_hmd_create(devices[index]);
	if (hmd == NULL) {
		return -1;
	}

	*out_xdevs = hmd;
	return 1;
}
