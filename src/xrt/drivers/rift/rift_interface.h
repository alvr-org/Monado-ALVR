// Copyright 2025, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Interface to Oculus Rift driver code.
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_rift
 */

#pragma once

#include "xrt/xrt_device.h"
#include "xrt/xrt_defines.h"
#include "xrt/xrt_prober.h"
#include "xrt/xrt_frame.h"

#include "tracking/t_time_sync.h"

#include "util/u_time.h"

#include "constellation/t_constellation_tracker.h"


#ifdef __cplusplus
extern "C" {
#endif


struct rift_hmd;

enum rift_variant
{
	RIFT_VARIANT_DK2,
	RIFT_VARIANT_CV1,
};

#define OCULUS_VR_VID 0x2833

#define OCULUS_DK2_PID 0x0021
#define OCULUS_DK2_SENSOR_PID 0x0201
#define OCULUS_CV1_PID 0x0031
#define OCULUS_CV1_SENSOR_PID 0x0211

#define RIFT_DK2_PRODUCT_STRING "Rift DK2"
#define RIFT_CV1_PRODUCT_STRING "Rift CV1"

/*!
 * Creates Rift HMD and related devices.
 *
 * @param hmd_dev       The HID device for the HMD.
 * @param radio_dev     The HID device for the radio, if present (CV1 only).
 * @param variant       The Rift variant (DK2 or CV1).
 * @param serial_number The serial number of the device.
 * @param xfctx         The frame context to use for the timing source functions. Non-optional.
 * @param out_hmd       Output pointer for the created rift_hmd struct.
 * @param out_xdevs     Output array for the created xrt_device pointers. The array must have space for at least 4
 *                      devices (HMD, left touch, right touch, remote).
 *
 * @return The number of devices created, or a negative value on error.
 */
int
rift_devices_create(struct os_hid_device *hmd_dev,
                    struct os_hid_device *radio_dev,
                    enum rift_variant variant,
                    const char *serial_number,
                    struct xrt_frame_context *xfctx,
                    struct rift_hmd **out_hmd,
                    struct xrt_device **out_xdevs);

bool
rift_get_radio_id(struct rift_hmd *hmd, uint8_t out_radio_id[5]);

/*!
 * Timestamps a camera frame with the time of the exposure that produced it, which is taken to be the exposure whose
 * IN report arrived within half a frame interval of one whole frame interval before the frame's first packet.
 *
 * @param user_data The @ref rift_hmd whose exposures the frame is matched against.
 * @param[out] timestamp Where the exposure time is written, only touched when this returns true.
 * @param frame_start_ns When the frame's first payload packet arrived, in local monotonic time.
 * @param pts       The PTS of the frame, in the camera's own clock. Only used for logging.
 *
 * @return True when an exposure time was written. False when no exposure lines up with the frame, in which case the
 *         caller should keep whatever timestamp it already had.
 */
bool
rift_hmd_frame_timestamp_callback(void *user_data,             //
                                  timepoint_ns *out_timestamp, //
                                  uint64_t *out_sequence_id,   //
                                  timepoint_ns frame_start_ns, //
                                  uint32_t pts);               //

int
rift_add_to_constellation_tracker(struct rift_hmd *hmd, struct t_constellation_tracker *tracker);

/*!
 * Gets the HMD's timing event source.
 *
 * @param hmd The rift_hmd to get the timing event source from.
 *
 * @return The timing event source for the HMD, or NULL if the timing source is not initialized.
 */
struct t_timing_event_source *
rift_hmd_get_timing_event_source(struct rift_hmd *hmd);

/*!
 * @dir drivers/rift
 *
 * @brief @ref drv_rift files.
 */

#ifdef __cplusplus
}
#endif
