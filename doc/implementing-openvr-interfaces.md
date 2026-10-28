# Implementing OpenVR Interfaces {#implementing-openvr-interfaces}

<!--
Copyright 2026, Beyley Cardellio
SPDX-License-Identifier: BSL-1.0
-->

[TOC]

This file describes some basic rules for implementing OpenVR interfaces
inside `st/openvr`.

## Versions

The "root" versions are always the latest versions in the OpenVR headers we
target, and all older interfaces are implemented on top of these root
versions, or are implemented on top of other older versions.

Old versions should be stored in the `interfaces/old/` directory, and should
be a single header file describing implementing the old version on top of the
root version. Most interfaces are very similar between versions, so most
functions can be implemented using the `Forward_` macros available in the
`openvr_forward_macros.h` header.

## State

All state should live in objects owned by the latest implemented version
of the IVRClientCore interface. No state should live in any other interfaces,
as we may have multiple interface versions active at the same time and
they should cleanly work together.

## Implementation placement

Logic more than a line or two of code, or which interacts with state directly
should be kept in abstracted objects such as `xrt::state_trackers::openvr::Compositor`.
However, this is not a hard and fast rule, and some smaller code may make sense
to keep within the interfaces. If any functions need to start having their own
local helper functions, or starts growing past "converting data to an internal
format, calling one function and converting back to what OpenVR expects",
that's a strong hint to move it to the abstractions.
