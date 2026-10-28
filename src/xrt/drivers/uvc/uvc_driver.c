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

#include "util/u_debug.h"
#include "util/u_frameserver.h"
#include "util/u_time.h"

#include "math/m_api.h"

#include "uvc_internal.h"

#include <assert.h>
#include <errno.h>
#include <libusb.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define UVC_TRACE(d, ...) U_LOG_IFL_T(d->log_level, __VA_ARGS__)
#define UVC_DEBUG(d, ...) U_LOG_IFL_D(d->log_level, __VA_ARGS__)
#define UVC_INFO(d, ...) U_LOG_IFL_I(d->log_level, __VA_ARGS__)
#define UVC_WARN(d, ...) U_LOG_IFL_W(d->log_level, __VA_ARGS__)
#define UVC_ERROR(d, ...) U_LOG_IFL_E(d->log_level, __VA_ARGS__)

#define SET_CUR 0x01
#define GET_CUR 0x81
#define TIMEOUT 1000

#define CONTROL_IFACE 0

#define VS_PROBE_CONTROL 1
#define VS_COMMIT_CONTROL 2

DEBUG_GET_ONCE_LOG_OPTION(uvc_log, "UVC_LOG", U_LOGGING_WARN)

#define VERBOSE_DEBUG 0

/*!
 * Cast to derived type.
 */
static inline struct uvc_fs *
uvc_fs(struct xrt_fs *xfs)
{
	return (struct uvc_fs *)xfs;
}

static void
uvc_stream_release_frame(struct xrt_frame *frame)
{
	struct uvc_fs *stream = (struct uvc_fs *)frame->owner;

	assert(frame->owner == stream);
	assert(stream->num_free_frames < stream->num_alloced_frames);
	// assert(frame->data_block_size == stream->frame_size);

	// Put the frame back on the free queue
	os_mutex_lock(&stream->frames_lock);
	stream->free_frames[stream->num_free_frames++] = frame;
	os_mutex_unlock(&stream->frames_lock);
}

int
uvc_set_cur(libusb_device_handle *dev,
            uint8_t usb_interface,
            uint8_t entity,
            uint8_t selector,
            void *data,
            uint16_t data_length)
{
	uint8_t bmRequestType = (uint8_t)LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE;
	uint8_t bRequest = SET_CUR;
	uint16_t wValue = selector << 8;
	uint16_t wIndex = entity << 8 | usb_interface;
	unsigned int timeout = TIMEOUT;
	int ret;

	ret = libusb_control_transfer(dev, bmRequestType, bRequest, wValue, wIndex, data, data_length, timeout);
	if (ret < 0) {
		U_LOG_IFL_E(U_LOGGING_ERROR, "Failed to set transfer SET CUR %u %u: %d %s", entity, selector, ret,
		            strerror(errno));
	}
	return ret;
}

int
uvc_get_cur(libusb_device_handle *dev,
            uint8_t usb_interface,
            uint8_t entity,
            uint8_t selector,
            void *data,
            uint16_t data_length)
{
	uint8_t bmRequestType = 0x80 | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE;
	uint8_t bRequest = GET_CUR;
	uint16_t wValue = selector << 8;
	uint16_t wIndex = entity << 8 | usb_interface;
	unsigned int timeout = TIMEOUT;
	int ret;

	ret = libusb_control_transfer(dev, bmRequestType, bRequest, wValue, wIndex, data, data_length, timeout);
	if (ret < 0) {
		U_LOG_IFL_E(U_LOGGING_ERROR, "Failed to transfer GET CUR %u %u: %d %s", entity, selector, ret,
		            strerror(errno));
	}
	return ret;
}

static void
process_payload(struct uvc_fs *stream, unsigned char *payload, size_t len)
{
	struct uvc_payload_header *h = (struct uvc_payload_header *)payload;
	size_t payload_len;
	int frame_id;
	uint32_t pts = (uint32_t)(-1);
#if VERBOSE_DEBUG
	uint32_t scr = (uint32_t)(-1);
	bool have_scr;
#endif
	bool error, have_pts, is_eof;

	if (len == 0 || len == 12) {
		return;
	}

	// ignore headers w/ unknown lengths
	if (h->bHeaderLength != 12) {
		// bad length
		if (h->bHeaderLength != 0) {
			UVC_ERROR(stream, "invalid UVC header: len %u/%u\n", h->bHeaderLength, (uint32_t)len);
		}

		return;
	}

	payload += h->bHeaderLength;
	payload_len = len - h->bHeaderLength;
	frame_id = h->bmHeaderInfo & 0x01;
	is_eof = (h->bmHeaderInfo & 0x02) != 0;
	have_pts = (h->bmHeaderInfo & 0x04) != 0;
#if VERBOSE_DEBUG
	have_scr = (h->bmHeaderInfo & 0x08) != 0;
#endif
	error = (h->bmHeaderInfo & 0x40) != 0;

	if (error) {
		UVC_ERROR(stream, "UVC frame error");
		return;
	}

	if (have_pts) {
		pts = __le32_to_cpu(h->dwPresentationTime);
		// Skip this warning for JPEG, where we only output when we see the frame change:
		if (stream->frame_collected != 0 && pts != stream->cur_pts &&
		    stream->parameters.format != XRT_FORMAT_MJPEG) {
			UVC_WARN(stream, "UVC PTS changed in-frame at %zu bytes. Lost %u hz", stream->frame_collected,
			         (pts - stream->cur_pts) * 1000);
			stream->cur_pts = pts;
		}
	}
#if VERBOSE_DEBUG
	if (have_scr) {
		scr = __le32_to_cpu(h->scrSourceClock);
	}
#endif

	// starting a new frame
	if (frame_id != stream->frame_id) {
		if (stream->frame_collected > 0) {
			UVC_WARN(stream, "UVC Dropping short frame: %zu < %zu (%zd lost)", stream->frame_collected,
			         stream->frame_size, stream->frame_size - stream->frame_collected);
		}

		timepoint_ns time = os_monotonic_get_ns();

		// Get a frame to capture into
		if (stream->cur_frame == NULL) {
			os_mutex_lock(&stream->frames_lock);
			if (stream->num_free_frames > 0) {
				stream->num_free_frames--;
				xrt_frame_reference(&stream->cur_frame, stream->free_frames[stream->num_free_frames]);
			}
			os_mutex_unlock(&stream->frames_lock);
		}

		stream->frame_id = frame_id;
		stream->cur_pts = pts;
		stream->frame_collected = 0;
		stream->skip_frame = false;

		if (stream->cur_frame == NULL) {
			if (stream->skip_frame_start == 0) {
				UVC_WARN(stream, "No frame provided for pixel data. Skipping frames");
				stream->skip_frame_start = time;
			}
			stream->skip_frame = true;
		}

		struct xrt_frame *frame = stream->cur_frame;

		if (frame) {
			// assert(frame->data_block_size >= stream->frame_size);

			frame->timestamp = time;
			frame->source_timestamp = pts;
			frame->source_sequence = stream->sequence_id++;
			frame->stride = stream->parameters.stride;
			frame->width = stream->parameters.width;
			frame->height = stream->parameters.height;
			frame->format = stream->parameters.format;

			// if we were provided a function to retrieve frame timestamps, try to call it to get
			// the timestamp of the frame, this is for things like DK2/CV1 where we want to set the
			// frame timestamp to be based on timestamps given by the device, rather than receive
			// time.
			timepoint_ns custom_timestamp;
			uint64_t custom_sequence_id = frame->source_sequence;
			if (stream->get_frame_timestamp &&
			    stream->get_frame_timestamp(stream->get_frame_timestamp_user_data, //
			                                &custom_timestamp,                     //
			                                &custom_sequence_id,                   //
			                                time,                                  //
			                                pts)) {                                //
				frame->timestamp = custom_timestamp;
				frame->source_sequence = custom_sequence_id;
			}

			if (stream->skip_frame_start != 0 && stream->cur_frame) {
				UVC_WARN(stream, "Got capture frame after %f sec",
				         time_ns_to_s(stream->cur_frame->timestamp - stream->skip_frame_start));
				stream->skip_frame_start = 0;
			}
		}

#if VERBOSE_DEBUG
		int64_t dt = 0;
		if (stream->cur_frame) {
			dt = time - stream->cur_frame->start_ts;
		}

		UVC_TRACE(stream, "UVC dt %f PTS %fhz SCR %fhz delta %d", (double)(dt) / (1000000000.0), pts, scr,
		          scr - pts);
#endif
	}

	if (stream->skip_frame || !stream->cur_frame) {
		return;
	}

	if (stream->frame_collected + payload_len > stream->frame_size) {
		UVC_ERROR(stream, "UVC frame buffer overflow: %zu + %zu > %zu", stream->frame_collected, payload_len,
		          stream->frame_size);
		return;
	}

	memcpy(stream->cur_frame->data + stream->frame_collected, payload, payload_len);
	stream->frame_collected += payload_len;

	// For JPEG frames, check if we just saw the EOF marker, so we can emit the frame immediately. Otherwise we end
	// up waiting a few ms longer until the next frame starts to see the frame_id change before we know that this
	// one finished
	if (stream->parameters.format == XRT_FORMAT_MJPEG && stream->frame_collected >= 2) {
		uint8_t *eof_marker = stream->cur_frame->data + stream->frame_collected - 2;
		if (eof_marker[0] == 0xFF && eof_marker[1] == 0xD9) {
			is_eof = true;
		}
	}

	if (stream->frame_collected == stream->frame_size || is_eof) {
		stream->cur_frame->size = stream->frame_collected;
		if (stream->sink) {
			xrt_sink_push_frame(stream->sink, stream->cur_frame);
		}
		u_sink_debug_push_frame(&stream->usd, stream->cur_frame);
		stream->frame_collected = 0;
		xrt_frame_reference(&stream->cur_frame, NULL);
	}

	// Always restart a frame after eof.
	// @note CV1 sensor never seems to set this bit, but others might in the future
	if (is_eof) {
		stream->frame_collected = 0;
	}
}

void
uvc_fs_set_source_timestamp_callback(struct xrt_fs *fs, get_frame_timestamp_t callback, void *user_data)
{
	struct uvc_fs *stream = uvc_fs(fs);

	stream->get_frame_timestamp = callback;
	stream->get_frame_timestamp_user_data = user_data;
}

static void
iso_transfer_cb(struct libusb_transfer *transfer)
{
	struct uvc_fs *stream = transfer->user_data;
	int ret;
	int i;

	/* Handle error conditions */
	if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
		if (transfer->status != LIBUSB_TRANSFER_CANCELLED)
			UVC_ERROR(stream, "USB transfer error: %u", transfer->status);
		stream->active_transfers--;
		return;
	}

	if (!stream->is_running) {
		/* Not resubmitting. Reduce transfer count */
		stream->active_transfers--;
		return;
	}

	/* Handle contained isochronous packets */
	for (i = 0; i < transfer->num_iso_packets; i++) {
		unsigned char *payload;
		size_t payload_len;

		payload = libusb_get_iso_packet_buffer_simple(transfer, i);
		payload_len = transfer->iso_packet_desc[i].actual_length;
		process_payload(stream, payload, payload_len);
	}

	/* Resubmit transfer */
	for (i = 0; i < 5; i++) {
		if (!stream->is_running) {
			/* Not resubmitting. Reduce transfer count */
			stream->active_transfers--;
			return;
		}

		/* Sometimes this fails, and we retry */
		ret = libusb_submit_transfer(transfer);
		if (ret >= 0) {
			break;
		}

		// sleep 500us between retries
		os_nanosleep(U_TIME_1US_IN_NS * 500);
	}

	if (ret < 0) {
		/* FIXME: Close and re-open this sensor */
		UVC_ERROR(stream, "Failed to resubmit USB after %d attempts", i);
		stream->active_transfers--;
	} else if (i > 0) {
		UVC_WARN(stream, "Resubmitted USB xfer after %d attempts", i + 1);
	}
}

static xrt_result_t
uvc_fs_enumerate_modes(struct xrt_fs *xfs, struct xrt_fs_mode **out_modes, uint32_t *out_count)
{
	struct uvc_fs *fs = uvc_fs(xfs);

	*out_modes = U_TYPED_ARRAY_CALLOC(struct xrt_fs_mode, *out_count);
	if ((*out_modes) == NULL) {
		return XRT_ERROR_ALLOCATION;
	}
	*out_count = 1;

	(**out_modes) = fs->mode;

	return XRT_SUCCESS;
}

static xrt_result_t
uvc_fs_configure_capture(struct xrt_fs *xfs, struct xrt_fs_capture_parameters *cp)
{
	// @todo
	return XRT_SUCCESS;
}

static xrt_result_t
uvc_fs_is_running(struct xrt_fs *xfs, bool *out_is_running)
{
	struct uvc_fs *fs = uvc_fs(xfs);

	(*out_is_running) = fs->is_running;

	return XRT_SUCCESS;
}

static xrt_result_t
uvc_fs_stream_stop(struct xrt_fs *xfs)
{
	int ret;
	struct uvc_fs *stream = uvc_fs(xfs);

	stream->is_running = false;

	for (size_t i = 0; i < stream->num_transfers; i++) {
		if (stream->transfer[i] != NULL) {
			libusb_cancel_transfer(stream->transfer[i]);
		}
	}

	// Wait for active transfers to finish
	int attempts = 0;
	while (stream->active_transfers > 0 && attempts < 50) {
		ret = libusb_handle_events_timeout_completed(stream->usb_ctx,
		                                             &(struct timeval){.tv_sec = 0, .tv_usec = 100000}, NULL);

		if (ret < 0 && ret != LIBUSB_ERROR_INTERRUPTED) {
			UVC_ERROR(stream, "libusb_handle_events failed: %d", ret);
			break;
		}
		attempts++;
	}

	if (stream->active_transfers > 0) {
		UVC_WARN(stream, "Timed out waiting for %zu USB transfers to complete", stream->active_transfers);
	}

	ret = libusb_set_interface_alt_setting(stream->devh, 1, 0);
	if (ret) {
		UVC_WARN(stream, "Failed to clear USB alt setting to 0 for sensor errno %d (%s)", errno,
		         strerror(errno));
	}

	// Free frames
	for (size_t i = 0; i < stream->num_alloced_frames; i++) {
		struct xrt_frame frame = stream->alloced_frames[i];
		free(frame.data);
	}
	free(stream->alloced_frames);
	free(stream->free_frames);

	return XRT_SUCCESS;
}

static xrt_result_t
uvc_fs_stream_start(struct xrt_fs *xfs,
                    struct xrt_frame_sink *xs,
                    enum xrt_fs_capture_type capture_type,
                    uint32_t descriptor_index)
{
	int ret;
	struct uvc_fs *stream = uvc_fs(xfs);

	assert(!stream->is_running);

	stream->sink = xs;

	// @todo
	(void)descriptor_index;
	(void)capture_type;

	// @todo some random number that is probably high enough, figure out some actual minimum
	size_t min_frames = 7;

	ret = libusb_set_interface_alt_setting(stream->devh, 1, stream->alt_setting);
	if (ret) {
		UVC_ERROR(stream, "Failed to set interface alt setting %d", stream->alt_setting);
		return XRT_ERROR_USB_COMMAND_FAILED;
	}

	stream->is_running = true;
	stream->cur_frame = NULL; // we use NULL to mean "no frame yet"

	// Allocate frames and put on the free list
	stream->alloced_frames = calloc(min_frames, sizeof(struct xrt_frame));
	stream->free_frames = calloc(min_frames, sizeof(struct xrt_frame *));

	for (size_t i = 0; i < min_frames; i++) {
		struct xrt_frame frame = {0};
		frame.data = calloc(1, stream->frame_size);
		frame.size = stream->frame_size;
		frame.stereo_format = XRT_STEREO_FORMAT_NONE;
		frame.format = stream->parameters.format;
		frame.stride = stream->parameters.stride;
		frame.destroy = uvc_stream_release_frame;

		frame.owner = stream;
		stream->alloced_frames[i] = frame;
		stream->free_frames[i] = &stream->alloced_frames[i];
	}
	stream->num_free_frames = stream->num_alloced_frames = min_frames;

	// Submit transfers
	for (size_t i = 0; i < stream->num_transfers; i++) {
		ret = libusb_submit_transfer(stream->transfer[i]);
		if (ret < 0) {
			UVC_ERROR(stream, "failed to submit iso transfer %zu. Error %d errno %d", i, ret, errno);
			stream->active_transfers = i;

			uvc_fs_stream_stop((struct xrt_fs *)stream);
			return XRT_ERROR_USB_COMMAND_FAILED;
		}
	}

	stream->active_transfers = stream->num_transfers;
	return XRT_SUCCESS;
}

static void
uvc_fs_node_break_apart(struct xrt_frame_node *node)
{
	struct uvc_fs *vid = container_of(node, struct uvc_fs, node);
	uvc_fs_stream_stop(&vid->base);
}

static void
uvc_fs_node_destroy(struct xrt_frame_node *node)
{
	struct uvc_fs *vid = container_of(node, struct uvc_fs, node);
	uvc_fs_destroy(&vid->base);
}

static int
uvc_get_descriptor_ascii(libusb_device_handle *devh, uint8_t index, unsigned char *buf, int buf_len)
{
	if (index == 0)
		return -1;

	int len = libusb_get_string_descriptor_ascii(devh, index, buf, buf_len);
	if (len < 0) {
		buf[0] = 0;
		return len;
	}

	// set the last byte to null terminator, no matter what
	buf[MIN(len, buf_len - 1)] = 0;

	return len;
}

int
uvc_fs_create(libusb_context *usb_ctx,
              libusb_device_handle *devh,
              const struct libusb_device_descriptor *desc,
              setup_stream_parameters_callback_t setup_stream_parameters_callback,
              post_init_callback_t post_init_callback,
              void *user_data,
              struct xrt_frame_context *xfctx,
              struct xrt_fs **out_stream)
{
	int ret;

	struct uvc_fs *stream = U_TYPED_CALLOC(struct uvc_fs);
	stream->log_level = debug_get_log_option_uvc_log();
	u_frameserver_populate_function_pointers(&stream->base);
	stream->base.enumerate_modes = uvc_fs_enumerate_modes;
	stream->base.configure_capture = uvc_fs_configure_capture;
	stream->base.stream_start = uvc_fs_stream_start;
	stream->base.stream_stop = uvc_fs_stream_stop;
	stream->base.is_running = uvc_fs_is_running;
	stream->node.break_apart = uvc_fs_node_break_apart;
	stream->node.destroy = uvc_fs_node_destroy;

	// we're just ignoring errors...
	uvc_get_descriptor_ascii(devh, desc->iProduct, (unsigned char *)stream->base.product,
	                         ARRAY_SIZE(stream->base.product));
	uvc_get_descriptor_ascii(devh, desc->iManufacturer, (unsigned char *)stream->base.manufacturer,
	                         ARRAY_SIZE(stream->base.manufacturer));
	uvc_get_descriptor_ascii(devh, desc->iSerialNumber, (unsigned char *)stream->base.serial,
	                         ARRAY_SIZE(stream->base.serial));

	ret = os_mutex_init(&stream->frames_lock);
	if (ret < 0) {
		UVC_ERROR(stream, "could not create frame mutex! reason %d", ret);
		goto error;
	}
	stream->usb_ctx = usb_ctx;
	stream->devh = devh;
	stream->is_running = false;

	// Skip the first frame
	stream->skip_frame = true;
	stream->skip_frame_start = 0;

	ret = libusb_set_auto_detach_kernel_driver(devh, 1);
	if (ret < 0) {
		UVC_ERROR(stream, "could not detach uvcvideo driver, reason %d", ret);
		goto error;
	}

	ret = libusb_claim_interface(devh, 0);
	if (ret < 0) {
		UVC_ERROR(stream, "could not claim control interface, reason %d", ret);
		goto error;
	}

	ret = libusb_claim_interface(devh, 1);
	if (ret < 0) {
		UVC_ERROR(stream, "could not claim UVC data interface, reason %d", ret);
		goto error;
	}

	bool is_usb2 = (desc->bcdUSB < 0x300);

	struct uvc_probe_commit_control control = {
	    .bFormatIndex = 1,
	    .bFrameIndex = 1,
	};

	size_t packet_size;
	int alt_setting;
	if (!setup_stream_parameters_callback(desc->idVendor, desc->idProduct, is_usb2, devh, &control,
	                                      &stream->parameters, &packet_size, &alt_setting, user_data)) {
		UVC_ERROR(stream, "Unknown / unhandled USB device VID/PID 0x%04x / 0x%04x", desc->idVendor,
		          desc->idProduct);
		ret = -1;
		goto error;
	}

	ret = uvc_set_cur(devh, 1, 0, VS_PROBE_CONTROL, &control, sizeof control);
	if (ret < 0) {
		UVC_ERROR(stream, "Failed to set PROBE");
		goto error;
	}

	ret = uvc_get_cur(devh, 1, 0, VS_PROBE_CONTROL, &control, sizeof control);
	if (ret < 0) {
		UVC_ERROR(stream, "failed to get PROBE");
		goto error;
	}

	ret = uvc_set_cur(devh, 1, 0, VS_COMMIT_CONTROL, &control, sizeof control);
	if (ret < 0) {
		UVC_ERROR(stream, "failed to set COMMIT");
		goto error;
	}

	if (post_init_callback && !post_init_callback(desc->idVendor, desc->idProduct, is_usb2, devh, user_data)) {
		UVC_ERROR(stream, "Failed to run postinit callback");
		goto error;
	}

	stream->alt_setting = alt_setting;
	stream->frame_size = stream->parameters.stride * stream->parameters.height;

	size_t num_packets = (stream->frame_size + packet_size - 1) / packet_size;
	stream->num_transfers = (num_packets + 31) / 32;
	num_packets = num_packets / stream->num_transfers;

	stream->transfer = malloc(stream->num_transfers * sizeof(*stream->transfer));
	if (!stream->transfer) {
		ret = -ENOMEM;
		UVC_ERROR(stream, "Failed to allocate stream transfers");
		goto error;
	}

	for (size_t i = 0; i < stream->num_transfers; i++) {
		stream->transfer[i] = libusb_alloc_transfer(num_packets);
		if (!stream->transfer[i]) {
			UVC_ERROR(stream, "failed to allocate isochronous transfer");
			ret = -ENOMEM;
			goto error;
		}

		uint8_t bEndpointAddress = 0x81;
		int transfer_size = num_packets * packet_size;
		void *buf = malloc(transfer_size);
		stream->transfer[i]->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

		libusb_fill_iso_transfer(stream->transfer[i], devh, bEndpointAddress, buf, transfer_size, num_packets,
		                         iso_transfer_cb, stream, TIMEOUT);
		libusb_set_iso_packet_lengths(stream->transfer[i], packet_size);
	}

	stream->mode = (struct xrt_fs_mode){
	    .format = stream->parameters.format,
	    .width = stream->parameters.width,
	    .height = stream->parameters.height,
	};

	// It's now safe to add it to the context.
	xrt_frame_context_add(xfctx, &stream->node);

	// Start the variable tracking after we know what device we have.
	u_sink_debug_init(&stream->usd);
	u_var_add_root(stream, "UVC Frameserver", true);
	u_var_add_ro_text(stream, stream->base.product, "Card");
	u_var_add_log_level(stream, &stream->log_level, "Log Level");
	u_var_add_sink_debug(stream, &stream->usd, "Output");

	*out_stream = &stream->base;
	return 0;

error:
	free(stream);
	return ret;
}

int
uvc_fs_destroy(struct xrt_fs *xfs)
{
	struct uvc_fs *stream = uvc_fs(xfs);

	u_var_remove_root(stream);

	assert(!stream->is_running);

	if (stream->transfer != NULL) {
		for (size_t i = 0; i < stream->num_transfers; i++) {
			if (stream->transfer[i] != NULL) {
				libusb_free_transfer(stream->transfer[i]);
				stream->transfer[i] = NULL;
			}
		}
		free(stream->transfer);
		stream->transfer = NULL;
	}

	os_mutex_destroy(&stream->frames_lock);

	return 0;
}
