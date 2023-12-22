// Copyright 2019, Collabora, Ltd.
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Prober code to dump information.
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @ingroup st_prober
 */

#include "xrt/xrt_config_os.h"
#include "xrt/xrt_config_have.h"

#include "util/u_misc.h"
#include "util/u_pretty_print.h"

#include "p_prober.h"

#include <stdio.h>
#include <string.h>
#include <inttypes.h>


#define P(...)                                                                                                         \
	do {                                                                                                           \
		u_pp(dg, __VA_ARGS__);                                                                                 \
	} while (false)

#define PT(fmt, ...) P("\t" fmt "\n", __VA_ARGS__)
#define PTT(fmt, ...) P("\t\t" fmt "\n", __VA_ARGS__)

static int
print_ports(char *tmp, size_t size, uint8_t *ports, int num)
{
	switch (num) {
	case 1: {
		snprintf(tmp, size, "%i", ports[0]);
		return 1;
	}
	case 2: {
		snprintf(tmp, size, "%i.%i", ports[0], ports[1]);
		return 1;
	}
	case 3: {
		snprintf(tmp, size, "%i.%i.%i", ports[0], ports[1], ports[2]);
		return 1;
	}
	case 4: {
		snprintf(tmp, size, "%i.%i.%i.%i", ports[0], ports[1], ports[2], ports[3]);
		return 1;
	}
	case 5: {
		snprintf(tmp, size, "%i.%i.%i.%i.%i", ports[0], ports[1], ports[2], ports[3], ports[4]);
		return 1;
	}
	case 6: {
		snprintf(tmp, size, "%i.%i.%i.%i.%i.%i", ports[0], ports[1], ports[2], ports[3], ports[4], ports[5]);
		return 1;
	}
	case 7: {
		snprintf(tmp, size, "%i.%i.%i.%i.%i.%i.%i", ports[0], ports[1], ports[2], ports[3], ports[4], ports[5],
		         ports[6]);
		return 1;
	}
	default: return 0;
	}
}


/*
 *
 * "Exported" functions.
 *
 */

void
p_dump_device(struct prober *p, struct prober_device *pdev, int id, bool use_stdout)
{
	struct u_pp_sink_stack_only sink;
	u_pp_delegate_t dg = u_pp_sink_stack_only_init(&sink);

	char tmp[1024];

	if (pdev->usb.bus != 0 && pdev->usb.addr == 0 && pdev->base.vendor_id != 0 && pdev->base.product_id == 0) {
		return;
	}

	PT("% 3i: 0x%04x:0x%04x", id, pdev->base.vendor_id, pdev->base.product_id);
	PTT("ptr:              %p", (void *)pdev);
	PTT("usb_dev_class:    %02x", pdev->base.usb_dev_class);


	if (pdev->usb.serial != NULL || pdev->usb.product != NULL || pdev->usb.manufacturer != NULL) {
		PTT("usb.product:      %s", pdev->usb.product);
		PTT("usb.manufacturer: %s", pdev->usb.manufacturer);
		PTT("usb.serial:       %s", pdev->usb.serial);
	}

	if (pdev->usb.bus != 0 || pdev->usb.addr != 0) {
		PTT("usb.bus:          %i", pdev->usb.bus);
		PTT("usb.addr:         %i", pdev->usb.addr);
	}

	if (pdev->bluetooth.id != 0) {
		PTT("bluetooth.id:     %012" PRIx64 "", pdev->bluetooth.id);
	}

	int num = pdev->usb.num_ports;
	if (print_ports(tmp, ARRAY_SIZE(tmp), pdev->usb.ports, num)) {
		PTT("port%s            %s", num > 1 ? "s:" : ": ", tmp);
	}

#ifdef XRT_HAVE_LIBUSB
	if (pdev->usb.dev != NULL) {
		PTT("libusb:           %p", (void *)pdev->usb.dev);
	}
#endif

#ifdef XRT_HAVE_LIBUVC
	uvc_device_t *uvc_dev = pdev->uvc.dev;
	if (uvc_dev != NULL) {
		struct uvc_device_descriptor *desc;

		PTT("libuvc:           %p", (void *)uvc_dev);

		uvc_get_device_descriptor(uvc_dev, &desc);

		if (desc->product != NULL) {

			PTT("product:          '%s'", desc->product);
		}
		if (desc->manufacturer != NULL) {

			PTT("manufacturer:     '%s'", desc->manufacturer);
		}
		if (desc->serialNumber != NULL) {

			PTT("serial:           '%s'", desc->serialNumber);
		}

		uvc_free_device_descriptor(desc);
		desc = NULL;
	}
#endif

#ifdef XRT_HAVE_V4L2
	for (size_t j = 0; j < pdev->num_v4ls; j++) {
		struct prober_v4l *v4l = &pdev->v4ls[j];

		PTT("v4l.iface:        %i", (int)v4l->usb_iface);
		PTT("v4l.index:        %i", (int)v4l->v4l_index);
		PTT("v4l.path:         '%s'", v4l->path);
	}
#endif

#ifdef XRT_OS_LINUX
	for (size_t j = 0; j < pdev->num_hidraws; j++) {
		struct prober_hidraw *hidraw = &pdev->hidraws[j];

		PTT("hidraw.iface:     %i", (int)hidraw->interface);
		PTT("hidraw.path:      '%s'", hidraw->path);
	}
#endif

	if (use_stdout) {
		printf("%s", sink.buffer);
	} else {
		U_LOG_RAW("%s", sink.buffer);
	}
}
