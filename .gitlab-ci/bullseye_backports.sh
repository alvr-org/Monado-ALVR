#!/bin/sh
# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: 2024, Collabora, Ltd. and the Monado contributors

set -e

(
	echo 'deb https://deb.debian.org/debian bullseye-backports main contrib' > /etc/apt/sources.list.d/backports.list
	apt-get update && apt-get upgrade
	apt-get install --no-install-recommends -y -t bullseye-backports cmake
)
