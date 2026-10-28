// Copyright 2019, Collabora, Ltd.
// Copyright 2026, Beyley Cardellio
// SPDX-License-Identifier: BSL-1.0
/*!
 * @file
 * @brief  Serial implementation based on Linux termios
 * @author Jakob Bornecrantz <jakob@collabora.com>
 * @author Rylie Pavlik <rylie.pavlik@collabora.com>
 * @author Beyley Cardellio <ep1cm1n10n123@gmail.com>
 * @ingroup aux_os
 */

#include "xrt/xrt_config_os.h"

#ifndef XRT_OS_LINUX
#error Linux-specific file.
#endif

#include <util/u_misc.h>
#include <util/u_logging.h>

#include "os_serial.h"
#include "os/os_fd.h"

#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

struct serial_linux
{
	struct os_serial_device base;

	int fd;
};

static struct serial_linux *
serial_linux(struct os_serial_device *serial_dev)
{
	return (struct serial_linux *)serial_dev;
}

static ssize_t
linux_serial_read(struct os_serial_device *serial_dev, uint8_t *data, size_t size, int milliseconds)
{
	struct serial_linux *serial = serial_linux(serial_dev);

	return os_fd_read(serial->fd, data, size, milliseconds);
}

static ssize_t
linux_serial_write(struct os_serial_device *serial_dev, const uint8_t *data, size_t size)
{
	struct serial_linux *serial = serial_linux(serial_dev);

	return write(serial->fd, data, size);
}

static int
linux_serial_set_line_control(struct os_serial_device *serial_dev, bool dtr, bool rts)
{
	struct serial_linux *serial = serial_linux(serial_dev);
	int status;

	// Get the current modem status bits
	if (ioctl(serial->fd, TIOCMGET, &status) == -1) {
		return -errno;
	}

	if (dtr) {
		status |= TIOCM_DTR;
	} else {
		status &= ~TIOCM_DTR;
	}

	if (rts) {
		status |= TIOCM_RTS;
	} else {
		status &= ~TIOCM_RTS;
	}

	// Set the updated modem status bits
	if (ioctl(serial->fd, TIOCMSET, &status) == -1) {
		return -errno;
	}

	return 0;
}

static void
linux_serial_destroy(struct os_serial_device *serial_dev)
{
	struct serial_linux *serial = serial_linux(serial_dev);

	if (serial->fd >= 0) {
		close(serial->fd);
	}

	free(serial);
}

int
os_serial_open(const char *path, const struct os_serial_parameters *parameters, struct os_serial_device **out_serial)
{
	int fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK | O_CLOEXEC);
	if (fd < 0) {
		return -errno;
	}

	struct termios tty;
	if (tcgetattr(fd, &tty) != 0) {
		U_LOG_E("Failed to get terminal attributes: %s", strerror(errno));
		close(fd);
		return -errno;
	}

	// Clear parity bits
	tty.c_cflag &= ~(PARENB | PARODD);

	// Set according to PARODD mode
	switch (parameters->parity) {
	case OS_SERIAL_PARITY_NONE: break;
	case OS_SERIAL_PARITY_EVEN: tty.c_cflag |= PARENB; break;
	case OS_SERIAL_PARITY_ODD: tty.c_cflag |= PARENB | PARODD; break;
	default:
		U_LOG_E("Invalid parity mode: %d", parameters->parity);
		close(fd);
		return -EINVAL;
	}

	// Set stop bits
	switch (parameters->stop_bits) {
	case 1: tty.c_cflag &= ~CSTOPB; break;
	case 2: tty.c_cflag |= CSTOPB; break;
	default:
		U_LOG_E("Invalid stop bits: %d", parameters->stop_bits);
		close(fd);
		return -EINVAL;
	}

	// Clear data size bits
	tty.c_cflag &= ~CSIZE;
	// Set according to data bits
	switch (parameters->data_bits) {
	case 5: tty.c_cflag |= CS5; break;
	case 6: tty.c_cflag |= CS6; break;
	case 7: tty.c_cflag |= CS7; break;
	case 8: tty.c_cflag |= CS8; break;
	default:
		U_LOG_E("Invalid data bits: %d", parameters->data_bits);
		close(fd);
		return -EINVAL;
	}

	tty.c_cflag |= CREAD | CLOCAL; // Turn on READ & ignore ctrl lines (CLOCAL = 1)

	tty.c_lflag &= ~ICANON; // Disable canonical mode
	tty.c_lflag &= ~ECHO;   // Disable echo
	tty.c_lflag &= ~ECHOE;  // Disable erasure
	tty.c_lflag &= ~ECHONL; // Disable new-line echo
	tty.c_lflag &= ~ISIG;   // Disable interpretation of INTR, QUIT and SUSP

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // Turn off s/w flow ctrl
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
	                 ICRNL); // Disable any special handling of received bytes

	tty.c_oflag &= ~OPOST; // Prevent special interpretation of output bytes (e.g. newline chars)
	tty.c_oflag &= ~ONLCR; // Prevent conversion of newline to carriage return/line feed

	tty.c_cc[VTIME] = 1; // Set timeout to 0.1 seconds (1 decisecond)
	tty.c_cc[VMIN] = 0;  // Do not wait for minimum number of characters

	const speed_t baud_rate = (speed_t)parameters->baud_rate;

	// Set output speed
	if (cfsetospeed(&tty, baud_rate) != 0) {
		U_LOG_E("Failed to set output speed: %s", strerror(errno));
		close(fd);
		return -errno;
	}

	// Set input speed
	if (cfsetispeed(&tty, baud_rate) != 0) {
		U_LOG_E("Failed to set input speed: %s", strerror(errno));
		close(fd);
		return -errno;
	}

	// Set final attributes
	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
		U_LOG_E("Failed to set terminal attributes: %s", strerror(errno));
		close(fd);
		return -errno;
	}

	struct serial_linux *serial = U_TYPED_CALLOC(struct serial_linux);

	serial->base.read = linux_serial_read;
	serial->base.write = linux_serial_write;
	serial->base.set_line_control = linux_serial_set_line_control;
	serial->base.destroy = linux_serial_destroy;

	serial->fd = fd;

	(*out_serial) = &serial->base;

	return 0;
}
