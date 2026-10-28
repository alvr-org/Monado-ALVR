// Copyright 2019, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Frameserver interface for video drivers.
 * @author Pete Black <pblack@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup xrt_iface
 */

#pragma once

#include "xrt/xrt_frame.h"
#include "xrt/xrt_results.h"


#ifdef __cplusplus
extern "C" {
#endif

struct xrt_slam_sinks;

/*!
 * Controlling the camera capture parameters
 *
 * Used to configure cameras. since there is no guarantee every
 * frameserver will support any/all of these params, a 'best effort'
 * should be made to apply them. all numeric values are normalised
 * floats for broad applicability.
 *
 * @ingroup xrt_iface
 * @see xrt_fs
 */
struct xrt_fs_capture_parameters
{
	float gain;
	float exposure;
};

/*!
 * @see xrt_fs
 * @ingroup xrt_iface
 */
struct xrt_fs_mode
{
	uint32_t width;
	uint32_t height;
	enum xrt_format format;
	enum xrt_stereo_format stereo_format;
};

/*!
 * Enum describing which type of capture we are doing.
 * @see xrt_fs
 * @ingroup xrt_iface
 */
enum xrt_fs_capture_type
{
	XRT_FS_CAPTURE_TYPE_TRACKING = 0,
	XRT_FS_CAPTURE_TYPE_CALIBRATION = 1,
};

/*!
 * @interface xrt_fs
 * Frameserver that generates frames. Multiple subframes (like stereo and
 * mipmaps) can be generate in one frame.
 *
 * @ingroup xrt_iface
 */
struct xrt_fs
{
	//! Name of the frame server source, from the subsystem.
	char name[512];
	//! Frame server product identifier, matches the prober device.
	char product[32];
	//! Frame server manufacturer, matches the prober device.
	char manufacturer[32];
	//! Frame server serial number, matches the prober device.
	char serial[32];

	/*!
	 * All frames produced by this frameserver are tagged with this id.
	 */
	uint64_t source_id;

	/*!
	 * Enumerate all available modes that this frameserver supports.
	 *
	 * This *may* return no modes, which should be checked by the caller.
	 * In this case, the frameserver does not support streaming frames.
	 */
	xrt_result_t (*enumerate_modes)(struct xrt_fs *xfs, struct xrt_fs_mode **out_modes, uint32_t *out_count);

	/*!
	 * Set the capture parameters, not all types of parameters may be supported on some capture devices.
	 *
	 * This function *must* not fail from a subset of requested parameters being unsupported!
	 */
	xrt_result_t (*configure_capture)(struct xrt_fs *xfs, struct xrt_fs_capture_parameters *cp);

	/*!
	 * Start the capture stream.
	 *
	 * @param xfs              The frameserver
	 * @param xs               The sink for this frameserver to push frames into
	 * @param capture_type     The type of capture this is
	 * @param descriptor_index The mode the stream should start with.
	 */
	xrt_result_t (*stream_start)(struct xrt_fs *xfs,
	                             struct xrt_frame_sink *xs,
	                             enum xrt_fs_capture_type capture_type,
	                             uint32_t descriptor_index);

	/*!
	 * Setup SLAM sinks for all the sensors a SLAM implementation may supports and
	 * start the frame server stream. Use @ref xrt_fs::stream_start instead if
	 * you only need the image stream.
	 *
	 * @note Having this method extends the scope of the frameserver to something
	 * more akin to a generic data source instead of just serving frames.
	 *
	 * @todo Fix this incongruence. Maybe rename the interface to xrt_data_source.
	 */
	xrt_result_t (*slam_stream_start)(struct xrt_fs *xfs, struct xrt_slam_sinks *sinks);

	/*!
	 * Stop the capture stream.
	 */
	xrt_result_t (*stream_stop)(struct xrt_fs *xfs);

	/*!
	 * Is the capture stream running.
	 */
	xrt_result_t (*is_running)(struct xrt_fs *xfs, bool *out_is_running);
};


/*
 *
 * Inline functions.
 *
 */

/*!
 * @copydoc xrt_fs::enumerate_modes
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_fs
 */
static inline xrt_result_t
xrt_fs_enumerate_modes(struct xrt_fs *xfs, struct xrt_fs_mode **out_modes, uint32_t *out_count)
{
	return xfs->enumerate_modes(xfs, out_modes, out_count);
}

/*!
 * @copydoc xrt_fs::configure_capture
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_fs
 */
static inline xrt_result_t
xrt_fs_configure_capture(struct xrt_fs *xfs, struct xrt_fs_capture_parameters *cp)
{
	return xfs->configure_capture(xfs, cp);
}

/*!
 * @copydoc xrt_fs::stream_start
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_fs
 */
static inline xrt_result_t
xrt_fs_stream_start(struct xrt_fs *xfs,
                    struct xrt_frame_sink *xs,
                    enum xrt_fs_capture_type capture_type,
                    uint32_t descriptor_index)
{
	return xfs->stream_start(xfs, xs, capture_type, descriptor_index);
}

/*!
 * @copydoc xrt_fs::slam_stream_start
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_fs
 */
static inline xrt_result_t
xrt_fs_slam_stream_start(struct xrt_fs *xfs, struct xrt_slam_sinks *sinks)
{
	return xfs->slam_stream_start(xfs, sinks);
}

/*!
 * @copydoc xrt_fs::stream_stop
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_fs
 */
static inline xrt_result_t
xrt_fs_stream_stop(struct xrt_fs *xfs)
{
	return xfs->stream_stop(xfs);
}

/*!
 * @copydoc xrt_fs::is_running
 *
 * Helper for calling through the function pointer.
 *
 * @public @memberof xrt_fs
 */
static inline xrt_result_t
xrt_fs_is_running(struct xrt_fs *xfs, bool *out_is_running)
{
	return xfs->is_running(xfs, out_is_running);
}


#ifdef __cplusplus
}
#endif
