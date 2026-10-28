// Copyright 2017, Philipp Zabel
// Copyright 2019-2021, Jan Schmidt
// Copyright 2025-2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Userspace UVC frameserver implementation
 * @author Philipp Zabel <philipp.zabel@gmail.com>
 * @author Jan Schmidt <jan@centricular.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_uvc
 */

#pragma once

#include "os/os_threading.h"

#include "util/u_sink.h"
#include "util/u_logging.h"
#include "util/u_var.h"

#include "uvc_interface.h"


struct uvc_fs
{
	struct xrt_fs base;

	struct xrt_frame_node node;

	struct u_sink_debug usd;

	enum u_logging_level log_level;

	struct xrt_fs_mode mode;

	//! Target sink
	struct xrt_frame_sink *sink;

	struct uvc_stream_parameters parameters;

	struct os_mutex frames_lock;
	size_t num_free_frames;
	struct xrt_frame **free_frames;

	//! Frame data destination
	struct xrt_frame *cur_frame;

	//! Total size of a full frame in bytes
	size_t frame_size;
	//! Current frame ID
	int frame_id;
	//! Current PTS being accumulated
	uint32_t cur_pts;
	//! Number of bytes collected from the current frame
	size_t frame_collected;
	//! true if we're skipping the current frame
	bool skip_frame;
	//! The current sequence ID of this stream.
	uint64_t sequence_id;

	//! Time at which we started skipping frames
	timepoint_ns skip_frame_start;

	//! USB streaming alt_setting
	int alt_setting;

	size_t num_transfers;
	struct libusb_transfer **transfer;
	size_t active_transfers;

	libusb_context *usb_ctx;
	libusb_device_handle *devh;

	bool is_running;
	struct xrt_frame *alloced_frames;
	size_t num_alloced_frames;

	void *get_frame_timestamp_user_data;
	get_frame_timestamp_t get_frame_timestamp;
};
