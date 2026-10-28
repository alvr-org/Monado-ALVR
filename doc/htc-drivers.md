# HTC Drivers {#htc-drivers}

<!--
Copyright 2026, Beyley Cardellio
SPDX-License-Identifier: BSL-1.0
-->

[TOC]

This file is meant to explain the design complexities of our "HTC Config"
headset handling code (`aux/htc`, `d/vp2`). It assumes a level of familiarity
with how Lighthouse HMDs function.

## aux/vive

Beginning with the original HTC Vive, this implements shareable routines
for Lighthouse HMDs, extending to headsets such as the Valve Index,
Bigscreen Beyond, HTC Vive Pro 1, etc.

At it's core is the Lighthouse JSON Config (known officially as
[The JSON File](https://github.com/ValveSoftware/openvr/wiki/The-JSON-File-(Lighthouse-Devices))),
parsed by `vive_config.c`. This contains most of the information describing
how the headset operates, including display and IMU calibration, photodiode
positions for tracking, and much more.

## aux/htc

Some more modern HTC headsets which have Lighthouse support (Vive Pro 2,
Vive Cosmos + Vive Cosmos External Tracking Faceplate) have a second JSON
configuration file containing data separate from the Lighthouse config,
colloquially known as the "HTC Config". `aux/htc` implements shareable routines
relating to this configuration file.

This originates from headsets like the Vive Cosmos which by default
do not have any Lighthouse support, and so do not contain a Lighthouse
configuration file, necessitating something custom. However, even in later
Lighthouse headsets, like the Vive Pro 2, this side-channel configuration
file is still present and contains vital information for making the
headset functional.

At the core of `aux/htc`'s utility is an implementation of HTC's many custom
display distortion algorithms, most of which are much more complex than the
one used on standard lighthouse devices.

## d/vp2

This driver implements the Vive Pro 2 specific protocol, which is used to set
brightness, read the HTC Config, change microphone noise cancelling, display
mode, and more. The HTC config present in the Vive Pro 2 also contains extra
information not present in other HTC configs, so `d/vp2` has a structure which
extends the HTC Config with data only present on Vive Pro 2. It uses `aux/htc`
to parse out the shared data.

Importantly, it does not contain any code relevant to tracking, and does not
implement the `xrt_device` interface, this driver is intended to be
initialized by other drivers, to provide Vive Pro 2 specific functionality
that isn't covered by the Lighthouse protocol.

## d/vive

This is a driver which directly interfaces with true Lighthouse headsets to
drive the headset and implement the `xrt_device` interface. It uses `aux/vive`
to parse the Lighthouse configuration.

As of now, it does not have integrations for Vive Pro 2 or other HTC Config
headsets.

## d/survive

This driver uses the `libsurvive` library to implement the `xrt_device`
interface. When a Vive Pro 2 is detected, this driver imports `d/vp2` and
`aux/htc` to provide any functionality required, such as replacing the display
distortion algorithm with the one provided by `aux/htc`.

## d/steamvr_lh

`d/steamvr_lh` implements a runner for the [OpenVR driver interface](https://github.com/ValveSoftware/openvr/blob/master/docs/Driver_API_Documentation.md),
using it to implement the `xrt_device` interface, acting as a bridge between
OpenVR drivers and Monado.

Similar to `d/survive`, when a Vive Pro 2 is detected, any functionality not
available through normal means is implemented using `d/vp2` and `aux/htc`.
