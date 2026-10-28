// Copyright 2017, Philipp Zabel
// Copyright 2019-2021, Jan Schmidt
// Copyright 2025-2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Public interface for userspace UVC frameserver implementation
 * @author Philipp Zabel <philipp.zabel@gmail.com>
 * @author Jan Schmidt <jan@centricular.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup drv_uvc
 */

#pragma once

#include "xrt/xrt_byte_order.h"
#include "xrt/xrt_frame.h"
#include "xrt/xrt_frameserver.h"

#include "util/u_time.h"

#include <libusb.h>
#include <stdbool.h>
#include <stdint.h>


#pragma pack(push, 1)

struct uvc_payload_header
{
	uint8_t bHeaderLength;
	uint8_t bmHeaderInfo;
	__le32 dwPresentationTime;
	__le16 wSofCounter;
	__le32 scrSourceClock;
};
static_assert(sizeof(struct uvc_payload_header) == 0xC, "bad struct size");

struct uvc_probe_commit_control
{
	__le16 bmHint;
	uint8_t bFormatIndex;
	uint8_t bFrameIndex;
	__le32 dwFrameInterval;
	__le16 wKeyFrameRate;
	__le16 wPFrameRate;
	__le16 wCompQuality;
	__le16 wCompWindowSize;
	__le16 wDelay;
	__le32 dwMaxVideoFrameSize;
	__le32 dwMaxPayloadTransferSize;
	__le32 dwClockFrequency;
	uint8_t bmFramingInfo;
};
static_assert(sizeof(struct uvc_probe_commit_control) == 0x1F, "bad struct size");

#pragma pack(pop)

//! Called to get the timestamp of a specific frame, if a callee has a more precise way of timestamping frames.
typedef bool (*get_frame_timestamp_t)(
    void *user_data,             //< The user data pointer provided when setting the callback
    timepoint_ns *out_timestamp, //< The output timestamp of the frame
    uint64_t *out_sequence_id,   //< The output sequence ID of the frame
    timepoint_ns frame_start_ns, //< When the frame's first payload packet arrived, in local monotonic time
    uint32_t pts                 //< The PTS value from the UVC payload header
);

struct uvc_stream_parameters
{
	enum xrt_format format;
	uint32_t width;
	uint32_t height;
	size_t stride;
};

struct uvc_fs;

int
uvc_set_cur(libusb_device_handle *devh,
            uint8_t usb_interface,
            uint8_t entity,
            uint8_t selector,
            void *data,
            uint16_t data_length);
int
uvc_get_cur(libusb_device_handle *devh,
            uint8_t usb_interface,
            uint8_t entity,
            uint8_t selector,
            void *data,
            uint16_t data_length);

//! Callback to setup stream parameters based on USB device parameters
typedef bool (*setup_stream_parameters_callback_t)(
    uint16_t vid,                             //< The VID of the USB device
    uint16_t pid,                             //< The PID of the USB device
    bool is_usb2,                             //< True if the device is USB2, false if USB3
    libusb_device_handle *devh,               //< The libusb device handle
    struct uvc_probe_commit_control *control, //< The output probe/commit control structure
    struct uvc_stream_parameters *parameters, //< The output stream parameters
    size_t *packet_size,                      //< The output packet size
    int *alt_setting,                         //< The output alternate setting
    void *user_data                           //< The user data pointer provided when setting the callback
);

//! Called after initialization in-case any extra device-specific setup is required (like on CV1 sensors over USB2)
typedef bool (*post_init_callback_t)(uint16_t vid,               //< The VID of the USB device
                                     uint16_t pid,               //< The PID of the USB device
                                     bool is_usb2,               //< True if the device is USB2, false if USB3
                                     libusb_device_handle *devh, //< The libusb device handle
                                     void *user_data //< The user data pointer provided when setting the callback
);

int
uvc_fs_create(libusb_context *usb_ctx,
              libusb_device_handle *devh,
              const struct libusb_device_descriptor *desc,
              setup_stream_parameters_callback_t setup_stream_parameters_callback,
              post_init_callback_t post_init_callback,
              void *user_data,
              struct xrt_frame_context *xfctx,
              struct xrt_fs **stream);

int
uvc_fs_destroy(struct xrt_fs *stream);

void
uvc_fs_set_source_timestamp_callback(struct xrt_fs *stream, get_frame_timestamp_t callback, void *user_data);
