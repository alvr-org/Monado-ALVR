// Copyright 2020-2024, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  "auto-prober" for Sample HMD that can be autodetected but not through USB VID/PID.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup drv_alvr
 */

#include "xrt/xrt_prober.h"

#include "util/u_misc.h"

#include "alvr_interface.h"
#include <stdio.h>


/*!
 * @implements xrt_auto_prober
 */
struct alvr_auto_prober
{
	struct xrt_auto_prober base;
};

//! @private @memberof alvr_auto_prober
static inline struct alvr_auto_prober *
alvr_auto_prober(struct xrt_auto_prober *xap)
{
	return (struct alvr_auto_prober *)xap;
}

//! @private @memberof alvr_auto_prober
static void
alvr_auto_prober_destroy(struct xrt_auto_prober *p)
{
	struct alvr_auto_prober *ap = alvr_auto_prober(p);

	free(ap);
}

//! @public @memberof alvr_auto_prober
static int
alvr_auto_prober_autoprobe(struct xrt_auto_prober *xap,
                             cJSON *attached_data,
                             bool no_hmds,
                             struct xrt_prober *xp,
                             struct xrt_device **out_xdevs)
{
	struct alvr_auto_prober *ap = alvr_auto_prober(xap);
	(void)ap;

	// Do not create an HMD device if we are not looking for HMDs.
	if (no_hmds) {
		return 0;
	}

	out_xdevs[0] = alvr_hmd_create();
	return 1;
}

struct xrt_auto_prober *
alvr_create_auto_prober(void)
{
	struct alvr_auto_prober *ap = U_TYPED_CALLOC(struct alvr_auto_prober);
	ap->base.name = "ALVR HMD Auto-Prober";
	ap->base.destroy = alvr_auto_prober_destroy;
	ap->base.lelo_dallas_autoprobe = alvr_auto_prober_autoprobe;

	return &ap->base;
}
