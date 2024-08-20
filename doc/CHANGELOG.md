# Changelog for Monado {#CHANGELOG}

```txt
SPDX-License-Identifier: CC0-1.0

SPDX-FileCopyrightText: 2020-2024 Collabora, Ltd. and the Monado contributors
```

## Monado 24.0.0 (2024-06-07)

This is a substantial release, our first tagged release in three years. In that
intervening period, most Monado users have been using the `main` development
branch directly. We intend to release more regularly going forward, with very
limited backports of fixes to stable branches as needed.

Compared to 21.0.0, this release features, among many other changes,
substantially improved OpenXR conformance, support for additional extensions as
well as initial support for OpenXR 1.1 (intended to be conformant but not yet
submitted), asynchronous time warp in both the graphics-pipeline and
compute-pipeline compositors, equal composition layer support in both compositor
pipelines, improved multi-platform capabilities, optical hand tracking in some
environments, support for external visual-inertial tracking (VIT aka SLAM)
modules, and many driver additions and improvements. The maintainers would like
to express our gratitude to the Monado community for their collective efforts,
submission of merge requests and issues, mutual support, derived runtimes, and
documentation efforts on top of Monado over the past three years. We look
forward to keeping the momentum up!

- Major changes
  - Implement OpenXR 1.1
    ([!2194](https://gitlab.freedesktop.org/monado/monado/merge_requests/2194))
  - Added WinMR driver, it supports most headsets and controllers. Controllers can
    be connected both via host-Bluetooth and tunneled with the onboard radio chip.
    By default has 3DoF tracking, it can do 6DoF if used with the Basalt SLAM
    tracking software. Distortion is there but doesn't work perfectly on all
    hardware, best results is on Reverb G2.
    ([!774](https://gitlab.freedesktop.org/monado/monado/merge_requests/774),
    [!780](https://gitlab.freedesktop.org/monado/monado/merge_requests/780),
    [!782](https://gitlab.freedesktop.org/monado/monado/merge_requests/782),
    [!784](https://gitlab.freedesktop.org/monado/monado/merge_requests/784),
    [!850](https://gitlab.freedesktop.org/monado/monado/merge_requests/850),
    [!989](https://gitlab.freedesktop.org/monado/monado/merge_requests/989),
    [!990](https://gitlab.freedesktop.org/monado/monado/merge_requests/990),
    [!991](https://gitlab.freedesktop.org/monado/monado/merge_requests/991),
    [!999](https://gitlab.freedesktop.org/monado/monado/merge_requests/999),
    [!1000](https://gitlab.freedesktop.org/monado/monado/merge_requests/1000),
    [!1003](https://gitlab.freedesktop.org/monado/monado/merge_requests/1003),
    [!1006](https://gitlab.freedesktop.org/monado/monado/merge_requests/1006),
    [!1010](https://gitlab.freedesktop.org/monado/monado/merge_requests/1010),
    [!1011](https://gitlab.freedesktop.org/monado/monado/merge_requests/1011),
    [!1014](https://gitlab.freedesktop.org/monado/monado/merge_requests/1014),
    [!1018](https://gitlab.freedesktop.org/monado/monado/merge_requests/1018),
    [!1025](https://gitlab.freedesktop.org/monado/monado/merge_requests/1025),
    [!1030](https://gitlab.freedesktop.org/monado/monado/merge_requests/1030),
    [!1035](https://gitlab.freedesktop.org/monado/monado/merge_requests/1035),
    [!1041](https://gitlab.freedesktop.org/monado/monado/merge_requests/1041),
    [!1048](https://gitlab.freedesktop.org/monado/monado/merge_requests/1048),
    [!1051](https://gitlab.freedesktop.org/monado/monado/merge_requests/1051),
    [!1052](https://gitlab.freedesktop.org/monado/monado/merge_requests/1052),
    [!1055](https://gitlab.freedesktop.org/monado/monado/merge_requests/1055),
    [!1056](https://gitlab.freedesktop.org/monado/monado/merge_requests/1056),
    [!1060](https://gitlab.freedesktop.org/monado/monado/merge_requests/1060),
    [!1063](https://gitlab.freedesktop.org/monado/monado/merge_requests/1063),
    [!1088](https://gitlab.freedesktop.org/monado/monado/merge_requests/1088),
    [!1101](https://gitlab.freedesktop.org/monado/monado/merge_requests/1101),
    [!1103](https://gitlab.freedesktop.org/monado/monado/merge_requests/1103),
    [!1111](https://gitlab.freedesktop.org/monado/monado/merge_requests/1111),
    [!1134](https://gitlab.freedesktop.org/monado/monado/merge_requests/1134),
    [!1227](https://gitlab.freedesktop.org/monado/monado/merge_requests/1227),
    [!1234](https://gitlab.freedesktop.org/monado/monado/merge_requests/1234),
    [!1244](https://gitlab.freedesktop.org/monado/monado/merge_requests/1244),
    [!1264](https://gitlab.freedesktop.org/monado/monado/merge_requests/1264),
    [!1291](https://gitlab.freedesktop.org/monado/monado/merge_requests/1291),
    [!1334](https://gitlab.freedesktop.org/monado/monado/merge_requests/1334),
    [!1382](https://gitlab.freedesktop.org/monado/monado/merge_requests/1382),
    [!1418](https://gitlab.freedesktop.org/monado/monado/merge_requests/1418),
    [!1446](https://gitlab.freedesktop.org/monado/monado/merge_requests/1446),
    [!1498](https://gitlab.freedesktop.org/monado/monado/merge_requests/1498),
    [!1506](https://gitlab.freedesktop.org/monado/monado/merge_requests/1506),
    [!1550](https://gitlab.freedesktop.org/monado/monado/merge_requests/1550),
    [!1590](https://gitlab.freedesktop.org/monado/monado/merge_requests/1590),
    [!1596](https://gitlab.freedesktop.org/monado/monado/merge_requests/1596),
    [!1602](https://gitlab.freedesktop.org/monado/monado/merge_requests/1602),
    [!1619](https://gitlab.freedesktop.org/monado/monado/merge_requests/1619),
    [!1636](https://gitlab.freedesktop.org/monado/monado/merge_requests/1636),
    [!1643](https://gitlab.freedesktop.org/monado/monado/merge_requests/1643),
    [!1665](https://gitlab.freedesktop.org/monado/monado/merge_requests/1665),
    [!1719](https://gitlab.freedesktop.org/monado/monado/merge_requests/1719),
    [!1744](https://gitlab.freedesktop.org/monado/monado/merge_requests/1744),
    [!1752](https://gitlab.freedesktop.org/monado/monado/merge_requests/1752),
    [!1754](https://gitlab.freedesktop.org/monado/monado/merge_requests/1754),
    [!1797](https://gitlab.freedesktop.org/monado/monado/merge_requests/1797),
    [!1845](https://gitlab.freedesktop.org/monado/monado/merge_requests/1845),
    [!1852](https://gitlab.freedesktop.org/monado/monado/merge_requests/1852),
    [!1855](https://gitlab.freedesktop.org/monado/monado/merge_requests/1855),
    [!1857](https://gitlab.freedesktop.org/monado/monado/merge_requests/1857),
    [!1859](https://gitlab.freedesktop.org/monado/monado/merge_requests/1859))
  - New compute-shader based rendering backend in the compositor. Supports
    projection, quad, equirect2, cylinder layers. It is not enabled by default.
    It also supports ATW. On some hardware the use of a compute queue improves
    latency when pre-empting other GPU work.
    ([!873](https://gitlab.freedesktop.org/monado/monado/merge_requests/873),
    [!1517](https://gitlab.freedesktop.org/monado/monado/merge_requests/1517),
    [!841](https://gitlab.freedesktop.org/monado/monado/merge_requests/841),
    [!1998](https://gitlab.freedesktop.org/monado/monado/merge_requests/1998),
    [!2001](https://gitlab.freedesktop.org/monado/monado/merge_requests/2001))
  - Added [Tracy](https://github.com/wolfpld/tracy) as a supported tracing
    backend, it joins the [Perfetto](https://perfetto.dev/) backend. Tracy works
    on Windows, but doesn't support full system tracing or multi app as well as
    Perfetto.
    ([!1576](https://gitlab.freedesktop.org/monado/monado/merge_requests/1576),
    [!1577](https://gitlab.freedesktop.org/monado/monado/merge_requests/1577),
    [!1579](https://gitlab.freedesktop.org/monado/monado/merge_requests/1579),
    [!1598](https://gitlab.freedesktop.org/monado/monado/merge_requests/1598),
    [!1827](https://gitlab.freedesktop.org/monado/monado/merge_requests/1827))
  - Add driver for XREAL (formerly nreal) Air glasses, the device features 3dof
    tracking. Also support XREAL Air 2 and XREAL Air 2 Pro.
    ([!1798](https://gitlab.freedesktop.org/monado/monado/merge_requests/1798),
    [!1989](https://gitlab.freedesktop.org/monado/monado/merge_requests/1989),
    [!2150](https://gitlab.freedesktop.org/monado/monado/merge_requests/2150),
    [#330](https://gitlab.freedesktop.org/monado/monado/issues/330),
    [!2172](https://gitlab.freedesktop.org/monado/monado/merge_requests/2172))
  - xrt: Updates binding verify usage for new binding code gen and pass in enabled
    extensions status.
    ([!1896](https://gitlab.freedesktop.org/monado/monado/merge_requests/1896))
  - Add `libmonado` library, allows control of applications and devices. Exposed
    API follows semver and is semi-stable. Will never be changed in a backward
    incompatible way without increasing the major version. Provisions for easily
    and safely checking version is included, both at compile and runtime.
    ([!1908](https://gitlab.freedesktop.org/monado/monado/merge_requests/1908),
    [!2055](https://gitlab.freedesktop.org/monado/monado/merge_requests/2055),
    [!2099](https://gitlab.freedesktop.org/monado/monado/merge_requests/2099))
  - Add driver for Rokid and Rokid Max glasses, the devices features 3dof tracking.
    ([!1930](https://gitlab.freedesktop.org/monado/monado/merge_requests/1930))
  - Add new curated debug GUI, while not exposing all the of the knobs it presents
    those knobs in a more organized way compared to the advanced UI. The curated UI
    also features the readback sink in the background, making it possible for it
    to double as the peek window.
    ([!1941](https://gitlab.freedesktop.org/monado/monado/merge_requests/1941))
  - Graphics compositor: Add new graphics layer helper code, supports projection,
    quad, cylinder, and equirect2 layers. This path now completely replaces the old
    layer renderer that was in the main compositor, making it reusable.
    ([!1983](https://gitlab.freedesktop.org/monado/monado/merge_requests/1983),
    [!1994](https://gitlab.freedesktop.org/monado/monado/merge_requests/1994),
    [!1995](https://gitlab.freedesktop.org/monado/monado/merge_requests/1995),
    [#299](https://gitlab.freedesktop.org/monado/monado/issues/299),
    [!2026](https://gitlab.freedesktop.org/monado/monado/merge_requests/2026),
    [!2105](https://gitlab.freedesktop.org/monado/monado/merge_requests/2105))
  - Add hot-switch support for input devices like controllers and gamepads. This is
    not full hot-plug support, but instead only enables switching between devices
    created at start.
    ([!1992](https://gitlab.freedesktop.org/monado/monado/merge_requests/1992))
  - Add framework support for face tracking xrt-devices. XR_HTC_facial_tracking
    being the first face tracking extension supported and the conventions of this
    for expression weights that xrt-device/drivers can output.
    More will be added in the future.
    ([!2163](https://gitlab.freedesktop.org/monado/monado/merge_requests/2163),
    [!2166](https://gitlab.freedesktop.org/monado/monado/merge_requests/2166),
    [!2218](https://gitlab.freedesktop.org/monado/monado/merge_requests/2218))
  - Add body tracking xrt-devices framework and support for the XR_FB_body_tracking
    extension.
    ([!2178](https://gitlab.freedesktop.org/monado/monado/merge_requests/2178))
  - Implement experimental XR_MNDX_xdev_space extension
    ([!2211](https://gitlab.freedesktop.org/monado/monado/merge_requests/2211),
    [!2215](https://gitlab.freedesktop.org/monado/monado/merge_requests/2215))
- XRT Interface
  - Added frame timing code that when the underlying vulkan driver supports the
    VK_GOOGLE_display_timing extension greatly improves the timing accuracy of
    the compositor. Along with this tracing code was added to better help use
    understand what was happening during a frame.
    ([!697](https://gitlab.freedesktop.org/monado/monado/merge_requests/697))
  - Add functionality to disable individual drivers in the configuration file.
    ([!704](https://gitlab.freedesktop.org/monado/monado/merge_requests/704))
  - Return `xrt_result_t` from `xrt_gfx_provider_create_gl_egl`.
    ([!705](https://gitlab.freedesktop.org/monado/monado/merge_requests/705))
  - Add `XRT_ERROR_EGL_CONFIG_MISSING` error, to handle missing config from EGL
    compositor creation call.
    ([!705](https://gitlab.freedesktop.org/monado/monado/merge_requests/705),
    [!768](https://gitlab.freedesktop.org/monado/monado/merge_requests/768))
  - Add small helper function for pushing frames.
    ([!715](https://gitlab.freedesktop.org/monado/monado/merge_requests/715))
  - Add `xrt_compositor_fence` interface to handle service and client render
    synchronisation.
    ([!721](https://gitlab.freedesktop.org/monado/monado/merge_requests/721))
  - Add `XRT_ERROR_THREADING_INIT_FAILURE` a new threading related error code.
    ([!721](https://gitlab.freedesktop.org/monado/monado/merge_requests/721))
  - Add alternative functions to `xrt_compositor::wait_frame` called
    `xrt_compositor::predict_frame` and `xrt_compositor::mark_frame` these allow
    one to do frame timing without having to block on the service side.
    ([!721](https://gitlab.freedesktop.org/monado/monado/merge_requests/721))
  - Add `xrt_multi_compositor_control` that allows the `xrt_system_compositor` to
    expose a interface that the IPC layer can use to manage multiple clients
    without having to do the rendering. This allows us to move a lot of the code
    out the IPC layer and make it more about passing calls.
    ([!721](https://gitlab.freedesktop.org/monado/monado/merge_requests/721))
  - Pass `XrFrameEndInfo::displayTime` to `xrt_compositor::layer_begin` so that the
    compositor can correctly schedule frames, most importantly do not display them
    too early that might lead to stutter.
    ([!721](https://gitlab.freedesktop.org/monado/monado/merge_requests/721))
  - Make `xrt_swapchain` be reference counted. This will greatly help with
    handling swapchains for multiple clients in the compositor rendering pipeline
    where a client might go away while the compositor is using it.
    ([!723](https://gitlab.freedesktop.org/monado/monado/merge_requests/723),
    [!754](https://gitlab.freedesktop.org/monado/monado/merge_requests/754),
    [!807](https://gitlab.freedesktop.org/monado/monado/merge_requests/807))
  - Make `enum xrt_blend_mode` an array instead of a bitfield, so that drivers can
    specify which one is preferred.
    ([!749](https://gitlab.freedesktop.org/monado/monado/merge_requests/749))
  - Add new IPC session not created error `XRT_ERROR_IPC_SESSION_NOT_CREATED`, as
    some functions cannot be called without first creating a session.
    ([!768](https://gitlab.freedesktop.org/monado/monado/merge_requests/768))
  - Make eye_relation argument to xrt_device_get_view_pose const, more safety for
    everybody.
    ([!794](https://gitlab.freedesktop.org/monado/monado/merge_requests/794))
  - Add `XRT_ERROR_IPC_SESSION_ALREADY_CREATED` error message to signal that a
    session has already been created on this connection.
    ([!800](https://gitlab.freedesktop.org/monado/monado/merge_requests/800))
  - Add a conventions page.
    ([!810](https://gitlab.freedesktop.org/monado/monado/merge_requests/810),
    [#61](https://gitlab.freedesktop.org/monado/monado/issues/61))
  - Send down sub-image offsets and sizes in normalised form, this makes it so that
    compositor does not need to track the size of swapchains.
    ([!847](https://gitlab.freedesktop.org/monado/monado/merge_requests/847))
  - Add `use_dedicated_allocation` field to `xrt_image_native` struct to track if
    dedicated allocation is required.
    ([!867](https://gitlab.freedesktop.org/monado/monado/merge_requests/867))
  - Add xrt_vec3_f64 struct.
    ([!870](https://gitlab.freedesktop.org/monado/monado/merge_requests/870))
  - Make `xrt_system_compositor_info::supported_blend_modes` an array with an
    adjacent count field.
    ([!975](https://gitlab.freedesktop.org/monado/monado/merge_requests/975))
  - Rename all `num_` parameters and fields in the interface, typically to `_count`
    but sometimes `_capacity`, to match OpenXR convention.
    ([!977](https://gitlab.freedesktop.org/monado/monado/merge_requests/977))
  - Remove `xrt_view::display::{w|h}_meters`, they are not used anywhere.
    ([!1054](https://gitlab.freedesktop.org/monado/monado/merge_requests/1054))
  - Rename `xrt_space_graph` (and related `m_space_graph_*` functions in
    `m_space.h`) to `xrt_relation_chain` to more accurately reflect the function of
    this structure.
    ([!1092](https://gitlab.freedesktop.org/monado/monado/merge_requests/1092))
  - Add the ability for `xrt_device` to dynamically change the fov of the views
    return back to the applications. We do this by addning a new function called
    `xrt_device::get_view_poses` and removing the old one. This function now
    returns view poses, fovs and the observer position in one go.
    ([!1105](https://gitlab.freedesktop.org/monado/monado/merge_requests/1105))
  - Add `XRT_TIMEOUT` to be used for operations that can timeout, same value as
    `VK_TIMEOUT` for extra compatibility.
    ([!1147](https://gitlab.freedesktop.org/monado/monado/merge_requests/1147))
  - Make it possible when creating the Vulkan client compositor to say if timeline
    semaphores has been enabled.
    ([!1164](https://gitlab.freedesktop.org/monado/monado/merge_requests/1164))
  - Add `xrt_compositor_semaphore` object, add interfaces to `xrt_compositor` for
    creating the new semaphore object. Also add interface for passing in semaphore
    to `xrt_compositor::layer_commit`. Added support for these interface through
    the whole Monado stack.
    ([!1164](https://gitlab.freedesktop.org/monado/monado/merge_requests/1164))
  - Add `XRT_CHECK_RESULT` define to be used for ensure that the result from
    functions are read and acted upon.
    ([!1166](https://gitlab.freedesktop.org/monado/monado/merge_requests/1166))
  - xrt: Add new `XRT_TRACING` environmental variable to control if tracing is
    enabled or not, this is to work around crashes in Perfetto when running the
    CTS.
    ([!1231](https://gitlab.freedesktop.org/monado/monado/merge_requests/1231))
  - Introduce `xrt_uuid_t` and `xrt_luid_t` structs and add these as field to
    `xrt_system_compositor_info` to more correctly transport UUID and LUID values.
    ([!1250](https://gitlab.freedesktop.org/monado/monado/merge_requests/1250),
    [!1257](https://gitlab.freedesktop.org/monado/monado/merge_requests/1257),
    [!1259](https://gitlab.freedesktop.org/monado/monado/merge_requests/1259))
  - xrt: Add xrt_swapchain_create_properties to allow client compositors and IPC
    code to query the main compositor how many images to create on a swapchain,
    and provide an extension point for more properties as needed.
    ([!1258](https://gitlab.freedesktop.org/monado/monado/merge_requests/1258))
  - Introduce `xrt_system_devices` struct and `xrt_instance_create_system` call.
    This is a prerequisite for builders, also has the added benefit of moving the
    logic of which devices has which role solely into the service/instance.
    ([!1275](https://gitlab.freedesktop.org/monado/monado/merge_requests/1275),
    [!1279](https://gitlab.freedesktop.org/monado/monado/merge_requests/1279),
    [!1283](https://gitlab.freedesktop.org/monado/monado/merge_requests/1283),
    [!1285](https://gitlab.freedesktop.org/monado/monado/merge_requests/1285),
    [!1299](https://gitlab.freedesktop.org/monado/monado/merge_requests/1299))
  - Introduce `xrt_builder` struct and various functions on `xrt_prober`
    to support them. This is a follow up on the `xrt_system_devices` changes.
    These make it much easier to build more complex integrated systems like WinMR
    and Rift-S, and moves a lot of contextual configuration out of generic drivers
    like the hand-tracker code needing to know which device it was being used by.
    ([!1285](https://gitlab.freedesktop.org/monado/monado/merge_requests/1285),
    [!1299](https://gitlab.freedesktop.org/monado/monado/merge_requests/1299),
    [!1313](https://gitlab.freedesktop.org/monado/monado/merge_requests/1313))
  - Make an `xrt_builder` specifically for Lighthouse (vive, index, etc.), and
    remove Lighthouse devices from the legacy builder.
    ([!1296](https://gitlab.freedesktop.org/monado/monado/merge_requests/1296))
  - No longer include any util headers (in this case `u_time.h`), the XRT headers
    are supposed to be completely self contained. The include also messed with
    build
    refactoring in the auxiliary directory.
    ([!1328](https://gitlab.freedesktop.org/monado/monado/merge_requests/1328))
  - Mark haptic value in xrt_device_set_output as const.
    ([!1408](https://gitlab.freedesktop.org/monado/monado/merge_requests/1408))
  - Remove unused `xrt_layer_cube_data::image_array_index` field.
    ([!1421](https://gitlab.freedesktop.org/monado/monado/merge_requests/1421))
  - Add a single header for limits, like max number of swapchain images.
    ([!1553](https://gitlab.freedesktop.org/monado/monado/merge_requests/1553))
  - Add `XRT_IPC_SERVICE_PID_FILE_NAME` cmake variable to configure the name of pid
    file.
    ([!1567](https://gitlab.freedesktop.org/monado/monado/merge_requests/1567))
  - Add `XRT_OXR_RUNTIME_SUFFIX` cmake variable to configure the suffix of the
    output openXR library.
    ([!1567](https://gitlab.freedesktop.org/monado/monado/merge_requests/1567))
  - Remove the `xrt_gfx_native.h` as it is no longer needed, it has been replaced
    by `compositor/main` own interface file. In the past it was the state tracker
    or IPC layer that created the `xrt_system_compositor` directly by calling this
    function. But now it's the `xrt_instance`s responsibility, and it can pick
    which compositor to create. So this interface becomes less special and just
    one of many possible compositors implementations.
    ([!1569](https://gitlab.freedesktop.org/monado/monado/merge_requests/1569))
  - Added `XRT_STRUCT_INIT` define to help with headers and code that needs to
    compile as both C and C++ code. This is due to differences in default struct
    initialization.
    ([!1578](https://gitlab.freedesktop.org/monado/monado/merge_requests/1578))
  - Document using `XRT_INPUT_GENERIC_UNBOUNDED_SPACE_POSE` unbounded reference
    space.
    ([!1621](https://gitlab.freedesktop.org/monado/monado/merge_requests/1621))
  - Remote `xrt_prober_device::product_name` array, the value was only used
    internally by the prober. There is already a access function for the product
    name that is needed by USB, so make the interface less confusing.
    ([!1732](https://gitlab.freedesktop.org/monado/monado/merge_requests/1732))
  - Added functions to swapchain to explicitly do the barrier insertion. This
    pushes the handling of barrier calls into the OpenXR state tracker, while
    the changes are minimal for the client compositors they no longer have the
    responsibility to implicitly to insert a barrier when needed. Allows us to
    in the future support extensions.
    ([!1743](https://gitlab.freedesktop.org/monado/monado/merge_requests/1743))
  - Introduce `xrt_layer_frame_data` struct that holds per frame data for the
    `xrt_compositor` layer interface. This is a sibling to the `xrt_layer_data`
    struct which holds per layer data. Both are structs instead of arguments to
    make it easier to pass the needed data through the layers of Monado, and for
    easier extension further down the line.
    ([!1755](https://gitlab.freedesktop.org/monado/monado/merge_requests/1755))
  - Extend `xrt_swapchain_create_properties` to allow the main compositor
    request extra bits to be used beyond those requested by the OpenXR app. Some
    compositors might need extra usage bits set beyond just the constant sampled
    bit. This also ensures that images have exactly the same usages in both the
    compositor and app.
    ([!1763](https://gitlab.freedesktop.org/monado/monado/merge_requests/1763))
  - Add eye gaze enums to enable exposing eye gaze data to the application.
    ([!1836](https://gitlab.freedesktop.org/monado/monado/merge_requests/1836))
  - Add Gen 3.0 and Tundar vive tracker device names.
    ([!1860](https://gitlab.freedesktop.org/monado/monado/merge_requests/1860))
  - Add generic vive tracker input and output defines.
    ([!1860](https://gitlab.freedesktop.org/monado/monado/merge_requests/1860))
  - Add and passthrough enabled/disabled state of the `XR_EXT_hand_tracking`
    extension.
    ([!1890](https://gitlab.freedesktop.org/monado/monado/merge_requests/1890),
    [!1841](https://gitlab.freedesktop.org/monado/monado/merge_requests/1841),
    [!1890](https://gitlab.freedesktop.org/monado/monado/merge_requests/1890))
  - Add and passthrough enabled/disabled state of the `XR_EXT_eye_gaze_interaction`
    extension.
    ([!1890](https://gitlab.freedesktop.org/monado/monado/merge_requests/1890))
  - Add a new `xrt_input_name` entry, `XRT_INPUT_GENERIC_PALM_POSE` for
    `XR_EXT_palm_pose`.
    ([!1896](https://gitlab.freedesktop.org/monado/monado/merge_requests/1896))
  - general: Add support for `XR_EXT_hand_interaction` profile.
    ([!1901](https://gitlab.freedesktop.org/monado/monado/merge_requests/1901))
  - Add OPPO MR controller defines.
    ([!1904](https://gitlab.freedesktop.org/monado/monado/merge_requests/1904))
  - Add trailing commas to all enums, reduces the size of any future changes to
    those enums.
    ([!1907](https://gitlab.freedesktop.org/monado/monado/merge_requests/1907))
  - Add new `xrt_device_name` enum entry for `XR_EXT_hand_interaction`.
    ([!1915](https://gitlab.freedesktop.org/monado/monado/merge_requests/1915))
  - Add performance settings interface, used to implement
    `XR_EXT_performance_settings`.
    ([!1936](https://gitlab.freedesktop.org/monado/monado/merge_requests/1936))
  - Add `xrt_thread_hint` enum and `xrt_compositor::set_thread_hint` function,
    this function lets us implement the
    [XR_KHR_android_thread_settings](https://registry.khronos.org/OpenXR/specs/1.0/man/html/XR_KHR_android_thread_settings.html)
    extension.
    ([!1951](https://gitlab.freedesktop.org/monado/monado/merge_requests/1951))
  - Add `xrt_limited_unique_id` and `xrt_limited_unique_id_t` types to donate a
    special id that is unique to the current process. Use that to decorate
    `xrt_swapchain_native` with a limited unique id, useful for caching of the
    `xrt_image_native` imports of swapchains and other objects.
    ([!1957](https://gitlab.freedesktop.org/monado/monado/merge_requests/1957),
    [!2066](https://gitlab.freedesktop.org/monado/monado/merge_requests/2066))
  - Add size limit for swapchain.
    ([!1964](https://gitlab.freedesktop.org/monado/monado/merge_requests/1964),
    [!2066](https://gitlab.freedesktop.org/monado/monado/merge_requests/2066))
  - Extend `xrt_system_devices` with dynamic roles for input devices and add
    function `xrt_system_devices_get_roles` to get updated devices.
    ([!1992](https://gitlab.freedesktop.org/monado/monado/merge_requests/1992),
    [!2019](https://gitlab.freedesktop.org/monado/monado/merge_requests/2019))
  - xrt/android: bump AGP to 8.1.0 and use latest google-java-format, spotless and
    svg-2-android-vector
    ([!2002](https://gitlab.freedesktop.org/monado/monado/merge_requests/2002))
  - Add `xrt_device` visibility mask interface, this is used to implement
    the OpenXR extension `XR_KHR_visibility_mask`.
    ([!2016](https://gitlab.freedesktop.org/monado/monado/merge_requests/2016),
    [!2032](https://gitlab.freedesktop.org/monado/monado/merge_requests/2032),
    [!2034](https://gitlab.freedesktop.org/monado/monado/merge_requests/2034),
    [!2067](https://gitlab.freedesktop.org/monado/monado/merge_requests/2067))
  - Extend `xrt_space_overseer` and other APIs to support LOCAL_FLOOR. In the
    Monado interface it is optional, but in OpenXR it is always required if the
    extension is supported or 1.1 is enabled.
    ([!2018](https://gitlab.freedesktop.org/monado/monado/merge_requests/2018),
    [!2048](https://gitlab.freedesktop.org/monado/monado/merge_requests/2048),
    [!1796](https://gitlab.freedesktop.org/monado/monado/merge_requests/1796))
  - xrt: Make `xrt_device::get_visibility_mask` return `xrt_return_t`.
    ([!2034](https://gitlab.freedesktop.org/monado/monado/merge_requests/2034))
  - Tidy the `xrt_device.h` file to make sure the destroy function is the last
    function on the device.
    ([!2038](https://gitlab.freedesktop.org/monado/monado/merge_requests/2038))
  - Add `XRT_ERROR_DEVICE_FUNCTION_NOT_IMPLEMENTED` error message, used to signal
    when a function that isn't implemented is called. It is not meant to query the
    availability of the function or feature, only a error condition on bad code.
    ([!2039](https://gitlab.freedesktop.org/monado/monado/merge_requests/2039))
  - handles: Add defines to characterize the behavior of the Vulkan graphics buffer
    import functionality: on most platforms, the import consumes the reference, but
    on some it just increases a ref count/clones the reference.
    ([!2042](https://gitlab.freedesktop.org/monado/monado/merge_requests/2042))
  - xrt: Improve the xrt_reference helper functions, making them more clear and
    better documented.
    ([!2047](https://gitlab.freedesktop.org/monado/monado/merge_requests/2047))
  - Add UNBOUNDED generic space poses, and re-order some of the
    generic inputs.
    ([!2048](https://gitlab.freedesktop.org/monado/monado/merge_requests/2048))
  - Add reference space usage information, this lets the space overseer know when
    a space has been used or no longer used. This allows for doing various things
    like recentering when an application starts, or stopping tracking of the floor
    if the stage or LOCAL_FLOOR space isn't used.
    ([!2048](https://gitlab.freedesktop.org/monado/monado/merge_requests/2048),
    [!2048](https://gitlab.freedesktop.org/monado/monado/merge_requests/2048),
    [!2092](https://gitlab.freedesktop.org/monado/monado/merge_requests/2092))
  - Fix graphics includes in the xrt_openxr_includes.h header.
    ([!2049](https://gitlab.freedesktop.org/monado/monado/merge_requests/2049))
  - Use uint32_t instead of int64_t for xrt_swapchain_create_info::format.
    ([!2049](https://gitlab.freedesktop.org/monado/monado/merge_requests/2049))
  - Add formats list to xrt_swapchain_create_info, used to implement the OpenXR
    extension XR_KHR_vulkan_swapchain_format_list.
    ([!2049](https://gitlab.freedesktop.org/monado/monado/merge_requests/2049))
  - For display refresh info add hz prefix, follow code style and add limit to
    refresh rate array.
    ([!2051](https://gitlab.freedesktop.org/monado/monado/merge_requests/2051))
  - Add new API in xrt_compositor and xrt_multi_compositor_control interfaces for
    display refresh rate setting and getting, used to implement
    XR_FB_display_refresh_rate.
    ([!2051](https://gitlab.freedesktop.org/monado/monado/merge_requests/2051))
  - Add new recenter function on `xrt_space_overseer` and new result code for when
    recentering isn't supported.
    ([!2055](https://gitlab.freedesktop.org/monado/monado/merge_requests/2055))
  - Introduce two new objects `xrt_system`, `xrt_session`, `xrt_session_event` and
    `xrt_session_event_sink`. Along with two new error returns
    `XRT_ERROR_IPC_COMPOSITOR_NOT_CREATED` and
    `XRT_ERROR_COMPOSITOR_NOT_SUPPORTED`.
    Also moves the compositor events to the session.
    ([!2062](https://gitlab.freedesktop.org/monado/monado/merge_requests/2062))
  - Add `xrt_session_event_reference_space_change_pending` event, this is used to
    generate `XrEventDataReferenceSpaceChangePending` in OpenXR.
    ([!2081](https://gitlab.freedesktop.org/monado/monado/merge_requests/2081))
  - Pass down broadcast `xrt_session_event_sink` pointer to prober and builder when
    creating system, this is used to broadcast events to all sessions. Such as
    reference space change pending event.
    ([!2081](https://gitlab.freedesktop.org/monado/monado/merge_requests/2081))
  - xrt_defines: Added new define `XRT_MAX_VIEWS` to define the maximum number of
    views supported by the system. This is used to define the maximum number of
    views supported by the distortion calculation as well as other view-related
    arrays.
    ([!2086](https://gitlab.freedesktop.org/monado/monado/merge_requests/2086))
  - xrt_device: Added new function `xrt_device_get_view_configuration` to get the
    view configuration for a device. Array size is determined by `XRT_MAX_VIEWS`.
    ([!2086](https://gitlab.freedesktop.org/monado/monado/merge_requests/2086))
  - The runtime name is now picked up from the CMake project description.
    ([!2089](https://gitlab.freedesktop.org/monado/monado/merge_requests/2089))
  - Add `xrt_device::ref_space_usage` function to let an `xrt_device` know if a
    reference it's powering is being used or not by any client.
    ([!2091](https://gitlab.freedesktop.org/monado/monado/merge_requests/2091),
    [!2107](https://gitlab.freedesktop.org/monado/monado/merge_requests/2107))
  - Make it possible to get builders from prober, this is useful for generating
    reports for end-user debugging.
    ([!2094](https://gitlab.freedesktop.org/monado/monado/merge_requests/2094))
  - Make it possible to control where dump goes, this is useful for generating
    reports for end-user debugging.
    ([!2094](https://gitlab.freedesktop.org/monado/monado/merge_requests/2094))
  - Prevent systemd from activating the user unit in quick succession, if it
    crashes on startup. This keep the units from entering a failed state.
    ([!2116](https://gitlab.freedesktop.org/monado/monado/merge_requests/2116))
  - config: Add `XRT_OS_ANDROID_USE_AHB` for Android platform. If this macro is not
    defined, Vulkan memory will be used to create swapchains.
    ([!2117](https://gitlab.freedesktop.org/monado/monado/merge_requests/2117))
  - xrt: Support STAGE space to be tracked by xrt_devices and implement in
    steamvr_lh, survive and remote drivers.
    ([!2121](https://gitlab.freedesktop.org/monado/monado/merge_requests/2121))
  - Support overriding steamvr path with the `STEAMVR_PATH` environment variable.
    ([!2149](https://gitlab.freedesktop.org/monado/monado/merge_requests/2149))
  - Add `XRT_SPACE_BOUNDS_UNAVAILABLE` and
    `XRT_ERROR_COMPOSITOR_FUNCTION_NOT_IMPLEMENTED` enum,
    and `xrt_compositor::get_reference_bounds_rect` function to implement
    `xrGetReferenceSpaceBoundsRect`
    ([!2180](https://gitlab.freedesktop.org/monado/monado/merge_requests/2180))
  - Fix compile error in t_imu.cpp due to missing header
    ([!2182](https://gitlab.freedesktop.org/monado/monado/merge_requests/2182))
  - Generate the `enum xrt_input_name` from a macro to avoid code duplication
    ([!2191](https://gitlab.freedesktop.org/monado/monado/merge_requests/2191))
- State Trackers
  - OpenXR: Keep track of the union of each action set's action sub-action paths
    ([!458](https://gitlab.freedesktop.org/monado/monado/merge_requests/458))
  - OpenXR: Stricter path verification in `xrSyncActions`
    ([!458](https://gitlab.freedesktop.org/monado/monado/merge_requests/458))
  - OpenXR: Fix action state change/timestamp updates
    ([!458](https://gitlab.freedesktop.org/monado/monado/merge_requests/458))
  - OpenXR: Ignore XrSystemHandTrackingPropertiesEXT when hand tracking extension
    is not enabled.
    ([!688](https://gitlab.freedesktop.org/monado/monado/merge_requests/688))
  - OpenXR: Support EGL clients sending in no EGLConfig if the EGLDisplay supports
    EGL_KHR_no_config_context.
    ([!705](https://gitlab.freedesktop.org/monado/monado/merge_requests/705))
  - OpenXR: Use new multi compositor controls to set visibility and z_order if
    available. This is needed for when we are not in service mode.
    ([!759](https://gitlab.freedesktop.org/monado/monado/merge_requests/759))
  - OpenXR: Add prefix to gfx related session functions to improve sorting.
    ([!847](https://gitlab.freedesktop.org/monado/monado/merge_requests/847))
  - OpenXR: Break out end frame handling to its own file since it's so big.
    ([!847](https://gitlab.freedesktop.org/monado/monado/merge_requests/847))
  - OpenXR: Fill in normalised sub-image offsets and sizes.
    ([!847](https://gitlab.freedesktop.org/monado/monado/merge_requests/847))
  - OpenXR: Add support for XR_KHR_swapchain_usage_input_attachment_bit.
    ([!886](https://gitlab.freedesktop.org/monado/monado/merge_requests/886))
  - OpenXR: Implement a basic support XR_FB_display_refresh_rate that can report
    the current refresh rate.
    ([!897](https://gitlab.freedesktop.org/monado/monado/merge_requests/897))
  - OpenXR: Add support for RenderDoc frame capture of OpenXR applications between
    xrBeginFrame and xrEndFrame.
    ([!1126](https://gitlab.freedesktop.org/monado/monado/merge_requests/1126))
  - OpenXR: Add `XRT_CHECK_RESULT` to oxr space functions.
    ([!1166](https://gitlab.freedesktop.org/monado/monado/merge_requests/1166))
  - OpenXR: Ensure even if relation is not locatable return only valid data.
    ([!1166](https://gitlab.freedesktop.org/monado/monado/merge_requests/1166))
  - OpenXR: Improve logging in `xrLocateSpace`.
    ([!1166](https://gitlab.freedesktop.org/monado/monado/merge_requests/1166))
  - OpenXR: Do not expose the XR_EXT_debug_utils extension, none of the functions
    where given out but we still listed the extension to the loader. So we put it
    behind a feature config that is always set to off.
    ([!1312](https://gitlab.freedesktop.org/monado/monado/merge_requests/1312))
  - OpenXR: Make sure to init session fields as early as possible.
    ([!1353](https://gitlab.freedesktop.org/monado/monado/merge_requests/1353))
  - OpenXR: Update headers to 1.0.23.
    ([!1355](https://gitlab.freedesktop.org/monado/monado/merge_requests/1355))
  - OpenXR: Validate faceCount parameter of XrSwapchainCreateInfo.
    ([!1399](https://gitlab.freedesktop.org/monado/monado/merge_requests/1399))
  - OpenXR: Now does the barrier insertion explicitly, see XRT comment for this MR.
    ([!1743](https://gitlab.freedesktop.org/monado/monado/merge_requests/1743))
  - OpenXR: Refactor logging functions and use OutputDebugStringA on Windows.
    ([!1785](https://gitlab.freedesktop.org/monado/monado/merge_requests/1785))
  - OpenXR: Refactor `OXR_NO_PRINTING` env vars, split them in two (which is useful
    for Windows that has stderr and Debug console), and make stderr printing
    default off on Windows.
    ([!1785](https://gitlab.freedesktop.org/monado/monado/merge_requests/1785),
    [!1785](https://gitlab.freedesktop.org/monado/monado/merge_requests/1785),
    [!1889](https://gitlab.freedesktop.org/monado/monado/merge_requests/1889))
  - OpenXR: Set extensions earlier in instance init.
    ([!1796](https://gitlab.freedesktop.org/monado/monado/merge_requests/1796))
  - OpenXR: Refactor wait frame function to avoid setting state before we should.
    ([!1805](https://gitlab.freedesktop.org/monado/monado/merge_requests/1805))
  - OpenXR: Fix crashes when enabling headless extension but using graphics.
    ([!1808](https://gitlab.freedesktop.org/monado/monado/merge_requests/1808),
    [#98](https://gitlab.freedesktop.org/monado/monado/issues/98))
  - OpenXR: Add support for `XR_EXT_eye_gaze_interaction`.
    ([!1836](https://gitlab.freedesktop.org/monado/monado/merge_requests/1836),
    [!1867](https://gitlab.freedesktop.org/monado/monado/merge_requests/1867),
    [#269](https://gitlab.freedesktop.org/monado/monado/issues/269),
    [!2027](https://gitlab.freedesktop.org/monado/monado/merge_requests/2027))
  - OpenXR: Use truncating printf helpers from util.
    ([!1865](https://gitlab.freedesktop.org/monado/monado/merge_requests/1865))
  - OpenXR: Check that argument performanceCounter to
    xrConvertWin32PerformanceCounterToTimeKHR is valid.
    ([!1880](https://gitlab.freedesktop.org/monado/monado/merge_requests/1880))
  - OpenXR: Add missing cpp defines/checks for: `XR_HTCX_vive_tracker_interaction`
    and `XR_MNDX_hydra`.
    ([!1890](https://gitlab.freedesktop.org/monado/monado/merge_requests/1890))
  - OpenXR: Add disabled `XR_EXT_palm_pose`, no device driver actually support it
    yet hence added in a disabled state.
    ([!1896](https://gitlab.freedesktop.org/monado/monado/merge_requests/1896))
  - OpenXR: Add disabled `XR_MSFT_hand_intertaction`.
    The binding code has support for this extension, but the bindings are not
    used in any of the drivers so totally untested and would lead to the wrong
    expectations of the applications.
    ([!1896](https://gitlab.freedesktop.org/monado/monado/merge_requests/1896))
  - OpenXR: Remove non-reachable return for `oxr_xrPathToString`
    ([!1899](https://gitlab.freedesktop.org/monado/monado/merge_requests/1899))
  - OpenXR: Add support for `XR_EXT_hand_interaction` profile
    ([!1901](https://gitlab.freedesktop.org/monado/monado/merge_requests/1901),
    [!2027](https://gitlab.freedesktop.org/monado/monado/merge_requests/2027))
  - OpenXR: Add XR_MNDX_system_buttons extension to expose system buttons for
    controllers where they have been omitted.
    ([!1903](https://gitlab.freedesktop.org/monado/monado/merge_requests/1903))
  - OpenXR: Add OPPO MR controller extension.
    ([!1904](https://gitlab.freedesktop.org/monado/monado/merge_requests/1904))
  - OpenXR: Fix profile look for `XR_EXT_hand_interaction` while not breaking
    `XR_msft_hand_interaction` binding look-up
    ([!1915](https://gitlab.freedesktop.org/monado/monado/merge_requests/1915))
  - OpenXR: Implement `XR_EXT_performance_settings`.
    ([!1936](https://gitlab.freedesktop.org/monado/monado/merge_requests/1936))
  - OpenXR: Implementation of XR_KHR_android_thread_settings.
    ([!1951](https://gitlab.freedesktop.org/monado/monado/merge_requests/1951))
  - OpenXR: Verify size limit for swapchain.
    ([!1964](https://gitlab.freedesktop.org/monado/monado/merge_requests/1964),
    [!2066](https://gitlab.freedesktop.org/monado/monado/merge_requests/2066))
  - OpenXR: Prefer use of action ref in binding code, in other words make
    `oxr_action_attachment_bind` only use `oxr_action_ref` params.
    ([!1985](https://gitlab.freedesktop.org/monado/monado/merge_requests/1985))
  - OpenXR: Refactor device getters.
    ([!1985](https://gitlab.freedesktop.org/monado/monado/merge_requests/1985))
  - OpenXR: Add support for new dynamic device roles. The bindings from actions to
    devices will be recalculated when decides change.
    ([!1992](https://gitlab.freedesktop.org/monado/monado/merge_requests/1992),
    [!1992](https://gitlab.freedesktop.org/monado/monado/merge_requests/1992),
    [!2073](https://gitlab.freedesktop.org/monado/monado/merge_requests/2073))
  - OpenXR: Route log output to Android logcat.
    ([!2003](https://gitlab.freedesktop.org/monado/monado/merge_requests/2003))
  - OpenXR: Implementation of XR_KHR_visibility_mask.
    ([!2016](https://gitlab.freedesktop.org/monado/monado/merge_requests/2016),
    [!2032](https://gitlab.freedesktop.org/monado/monado/merge_requests/2032),
    [!2027](https://gitlab.freedesktop.org/monado/monado/merge_requests/2027))
  - OpenXR: Refactor reference space validation and support checking, code now
    supports per system set of supported reference spaces.
    ([!2018](https://gitlab.freedesktop.org/monado/monado/merge_requests/2018))
  - OpenXR: Export local_floor if extension enabled and space is supported, since
    the extension is compile time we may break the space if the system actually
    doesn't support local_floor. Hopefully those cases should be rare.
    ([!2018](https://gitlab.freedesktop.org/monado/monado/merge_requests/2018),
    [!2018](https://gitlab.freedesktop.org/monado/monado/merge_requests/2018),
    [!2033](https://gitlab.freedesktop.org/monado/monado/merge_requests/2033),
    [!2027](https://gitlab.freedesktop.org/monado/monado/merge_requests/2027))
  - OpenXR: Export unbounded if extension enabled and space is supported, it's
    exposed via XR_MSFT_unbounded_reference_space.
    ([!2018](https://gitlab.freedesktop.org/monado/monado/merge_requests/2018),
    [!2018](https://gitlab.freedesktop.org/monado/monado/merge_requests/2018),
    [!2027](https://gitlab.freedesktop.org/monado/monado/merge_requests/2027))
  - OpenXR: Fix `xrSyncActions`' return code with no action sets.
    ([!2024](https://gitlab.freedesktop.org/monado/monado/merge_requests/2024))
  - OpenXR: Properly check all extension interaction profiles if enabled or
    supported in xrSuggestInteractionProfileBindings.
    ([!2027](https://gitlab.freedesktop.org/monado/monado/merge_requests/2027))
  - OpenXR: Make many more extensions build time options, doesn't change which
    are enabled by default. This lets runtimes using Monado control which
    extensions are exposed, this needs to be build time options because
    extensions are listed before a connection is made to the service.
    ([!2027](https://gitlab.freedesktop.org/monado/monado/merge_requests/2027))
  - OpenXR: Handle session not focused for action input and output and return
    XR_SESSION_NOT_FOCUSED where needed.
    ([!2035](https://gitlab.freedesktop.org/monado/monado/merge_requests/2035))
  - OpenXR: Use xrt_device function helper.
    ([!2038](https://gitlab.freedesktop.org/monado/monado/merge_requests/2038))
  - OpenXR: Add support for reference space usage.
    ([!2048](https://gitlab.freedesktop.org/monado/monado/merge_requests/2048))
  - OpenXR: Track which graphics API was used to create the session.
    ([!2049](https://gitlab.freedesktop.org/monado/monado/merge_requests/2049))
  - OpenXR: Implement XR_KHR_vulkan_swapchain_format_list.
    ([!2049](https://gitlab.freedesktop.org/monado/monado/merge_requests/2049),
    [!2049](https://gitlab.freedesktop.org/monado/monado/merge_requests/2049),
    [!2059](https://gitlab.freedesktop.org/monado/monado/merge_requests/2059),
    [!2083](https://gitlab.freedesktop.org/monado/monado/merge_requests/2083))
  - OpenXR: Complete implementation of extension XR_FB_display_refresh_rate.
    ([!2051](https://gitlab.freedesktop.org/monado/monado/merge_requests/2051),
    [!2054](https://gitlab.freedesktop.org/monado/monado/merge_requests/2054))
  - OpenXR: Add support for the new `xrt_system` and `xrt_session` objects.
    ([!2062](https://gitlab.freedesktop.org/monado/monado/merge_requests/2062))
  - OpenXR: Fix unhandled enum in switch statements.
    ([!2069](https://gitlab.freedesktop.org/monado/monado/merge_requests/2069),
    [!2063](https://gitlab.freedesktop.org/monado/monado/merge_requests/2063))
  - OpenXR: Transition headless session to FOCUSED on xrBeginSession as per the
    extension, this fixes actions not being active in headless sessions.
    ([!2072](https://gitlab.freedesktop.org/monado/monado/merge_requests/2072))
  - OpenXR: Tidy code a little bit and improve debugging of bindings.
    ([!2072](https://gitlab.freedesktop.org/monado/monado/merge_requests/2072))
  - OpenXR: Add guards around overlay event code and tidy event code.
    ([!2074](https://gitlab.freedesktop.org/monado/monado/merge_requests/2074))
  - OpenXR: Add extension XR_FB_composition_layer_image_layout.
    ([!2075](https://gitlab.freedesktop.org/monado/monado/merge_requests/2075))
  - OpenXR: Add `XR_KHR_composition_layer_color_scale_bias` support, disabled by
    default because Monado's main compositor doesn't support it.
    ([!2078](https://gitlab.freedesktop.org/monado/monado/merge_requests/2078),
    [!2082](https://gitlab.freedesktop.org/monado/monado/merge_requests/2082))
  - OpenXR: Use correct define to test for extension support.
    ([!2082](https://gitlab.freedesktop.org/monado/monado/merge_requests/2082))
  - OpenXR: Add `XR_FB_composition_layer_alpha_blend` support, disabled by
    default because Monado's main compositor doesn't support it.
    ([!2087](https://gitlab.freedesktop.org/monado/monado/merge_requests/2087))
  - OpenXR: Add extension XR_FB_composition_layer_settings.
    ([!2088](https://gitlab.freedesktop.org/monado/monado/merge_requests/2088))
  - OpenXR: Fix crashes on too many bindings to a single action.
    ([!2109](https://gitlab.freedesktop.org/monado/monado/merge_requests/2109))
  - OpenXR: Increase internal limit of bindings per action.
    ([!2109](https://gitlab.freedesktop.org/monado/monado/merge_requests/2109))
  - OpenXR: Switch to use new loader header.
    ([!2111](https://gitlab.freedesktop.org/monado/monado/merge_requests/2111))
  - OpenXR: Implement extension 'XR_FB_passthrough'.
    ([!2124](https://gitlab.freedesktop.org/monado/monado/merge_requests/2124))
  - OpenXR: Add extension XR_FB_composition_layer_depth_test.
    ([!2129](https://gitlab.freedesktop.org/monado/monado/merge_requests/2129))
  - OpenXR: Fix binding init/updates not being reset for any-pose-subaction paths
    ([!2133](https://gitlab.freedesktop.org/monado/monado/merge_requests/2133))
  - OpenXR: Fix `xrGetCurrentInteractionPath` returning incorrect paths for a
    particular hand becoming unbound
    ([!2133](https://gitlab.freedesktop.org/monado/monado/merge_requests/2133))
  - OpenXR: Fix invalid timestamps for action cache updates.
    ([!2146](https://gitlab.freedesktop.org/monado/monado/merge_requests/2146))
  - OpenXR: Get vendor id/name from server.
    ([!2151](https://gitlab.freedesktop.org/monado/monado/merge_requests/2151))
  - OpenXR: Avoid calling `oxr_action_cache_stop_output` every time when
    `xrSyncActions` is called.
    ([!2171](https://gitlab.freedesktop.org/monado/monado/merge_requests/2171))
  - OpenXR: Implement function 'xrGetReferenceSpaceBoundsRect'.
    ([!2180](https://gitlab.freedesktop.org/monado/monado/merge_requests/2180))
  - OpenXR: Fix null-pointer crash bug in `xrGetVisibilityMaskKHR` with in-process
    builds and replicates the same behaviour as out-of-process builds of falling
    back to a default implementation.
    ([!2210](https://gitlab.freedesktop.org/monado/monado/merge_requests/2210),
    [#375](https://gitlab.freedesktop.org/monado/monado/issues/375))
  - OpenXR: support `XrEventDataVisibilityMaskChangedKHR` for visibility mask
    ([!2217](https://gitlab.freedesktop.org/monado/monado/merge_requests/2217))
  - OpenXR: Ignore primaryViewConfigurationType with XR_MND_headless
    ([!2225](https://gitlab.freedesktop.org/monado/monado/merge_requests/2225))
  - gui: Add a GUI for recording videos from the Valve Index.
    ([!715](https://gitlab.freedesktop.org/monado/monado/merge_requests/715))
  - gui: Show git description in `monado-gui` window title.
    ([!830](https://gitlab.freedesktop.org/monado/monado/merge_requests/830))
  - gui: Add tracing support
    ([!858](https://gitlab.freedesktop.org/monado/monado/merge_requests/858))
  - gui: Various fixes for video handling, null checking and wrong argument orders.
    ([!859](https://gitlab.freedesktop.org/monado/monado/merge_requests/859))
  - gui: Add support to record from ELP 3D camera and select DepthAI camera
    to calibration.
    ([!859](https://gitlab.freedesktop.org/monado/monado/merge_requests/859))
  - gui: Support RGBA/RGBx and unusual image strides in record window.
    ([!1120](https://gitlab.freedesktop.org/monado/monado/merge_requests/1120))
  - gui: Add SW Ultrafast and SW Veryfast pipelines in record window.
    ([!1144](https://gitlab.freedesktop.org/monado/monado/merge_requests/1144))
  - gui: Make it possible to enter address and port for remote.
    ([!1356](https://gitlab.freedesktop.org/monado/monado/merge_requests/1356))
  - gui: Expose the new Index Controller UI for the remote driver.
    ([!1356](https://gitlab.freedesktop.org/monado/monado/merge_requests/1356))
  - gui: Add some tracing in the prober code.
    ([!1814](https://gitlab.freedesktop.org/monado/monado/merge_requests/1814))
  - gui: Tweaks for the sinks gui, opened by default and make possible to hide the
    header.
    ([!1827](https://gitlab.freedesktop.org/monado/monado/merge_requests/1827))
  - gui: General tidy and refactoring code to make it prettier.
    ([!1902](https://gitlab.freedesktop.org/monado/monado/merge_requests/1902))
  - gui: Add code to draw a image to the background of the main window.
    ([!1902](https://gitlab.freedesktop.org/monado/monado/merge_requests/1902))
  - gui: Fix warning with passing arrays.
    ([!1928](https://gitlab.freedesktop.org/monado/monado/merge_requests/1928))
  - gui: Rename OpenGL sink file, slightly improve thread safety and add note.
    ([!1957](https://gitlab.freedesktop.org/monado/monado/merge_requests/1957))
  - gui: Add infinitely fast trigger finger in remote UI.
    ([!2110](https://gitlab.freedesktop.org/monado/monado/merge_requests/2110))
  - gui/calibration: Skip mode selection if there's only one mode.
    ([!1074](https://gitlab.freedesktop.org/monado/monado/merge_requests/1074))
  - gui/calibration: Save/load parameters to file so that you don't have to change
    them every time.
    ([!1074](https://gitlab.freedesktop.org/monado/monado/merge_requests/1074))
  - oxr: Enable RenderDoc compiling on Android
    ([!2005](https://gitlab.freedesktop.org/monado/monado/merge_requests/2005))
  - oxr: Enable VK_EXT_debug_utils extension for client side on the platform that
    support it. Since it can not be reliably detected if the extension was enabled
    by the application on `XR_KHR_vulkan_enable` instead use the the environmental
    variable `OXR_DEBUG_FORCE_VK_DEBUG_UTILS` to force it on.
    ([!2005](https://gitlab.freedesktop.org/monado/monado/merge_requests/2005),
    [!2005](https://gitlab.freedesktop.org/monado/monado/merge_requests/2005),
    [!2044](https://gitlab.freedesktop.org/monado/monado/merge_requests/2044))
  - oxr: RenderDoc support on Vulkan/OpenGL ES client sides and Android platform,
    make rdc can be captured by the button in UI
    ([!2005](https://gitlab.freedesktop.org/monado/monado/merge_requests/2005))
  - prober: Minor fixes & tidy commits. Mostly around doc-comments and the string
    descriptor getter function.
    ([!686](https://gitlab.freedesktop.org/monado/monado/merge_requests/686))
  - prober: Change the default logging level to info so that people can see what
    drivers are disabled.
    ([!735](https://gitlab.freedesktop.org/monado/monado/merge_requests/735))
  - prober: Fix warnings found with GCC 13.
    ([!1921](https://gitlab.freedesktop.org/monado/monado/merge_requests/1921))
  - prober: Quit without crashing if no driver is available.
    ([!1996](https://gitlab.freedesktop.org/monado/monado/merge_requests/1996))
  - st/gui: Refactor a few OpenGL drawing code into helper.
    ([!1957](https://gitlab.freedesktop.org/monado/monado/merge_requests/1957))
  - steamvr_drv: Add support for new dynamic device roles.
    ([!1992](https://gitlab.freedesktop.org/monado/monado/merge_requests/1992))
  - steamvr_drv: Add support for the new `xrt_system` and `xrt_session` objects.
    ([!2062](https://gitlab.freedesktop.org/monado/monado/merge_requests/2062))
  - steamvr_drv: Fill HMD properties for games and apps to recognize it correctly.
    ([!2102](https://gitlab.freedesktop.org/monado/monado/merge_requests/2102))
- Drivers
  - Sample driver: Implement missing APIs.
    ([!2067](https://gitlab.freedesktop.org/monado/monado/merge_requests/2067),
    [!2135](https://gitlab.freedesktop.org/monado/monado/merge_requests/2135))
  - ULv5: Add new driver for UltraLeap v5 API for hand-tracking devices.
    ([!2064](https://gitlab.freedesktop.org/monado/monado/merge_requests/2064))
  - adjust the pose for VIT system and assumes basalt
    ([!2058](https://gitlab.freedesktop.org/monado/monado/merge_requests/2058))
  - all: Use `u_device_noop_update_inputs` helper for drivers with nothing in their
    update input function.
    ([!2039](https://gitlab.freedesktop.org/monado/monado/merge_requests/2039))
  - all: Standardize use of `u_device_get_view_poses` helper.
    ([!2039](https://gitlab.freedesktop.org/monado/monado/merge_requests/2039))
  - android_sensors: Fixed the issue of screen stuttering on some Android devices
    caused by failing to set the IMU event rate.
    ([!1912](https://gitlab.freedesktop.org/monado/monado/merge_requests/1912))
  - android_sensors: Set both orientation and position valid flags in the Android
    driver's `get_tracked_pose` callback. hello_xr, Unity and possibly other apps
    check the view pose flags for both position & orientation flags to be valid
    otherwise they invoke `xrEndFrame` with no layers set causing a constant gray
    screen.
    ([!2219](https://gitlab.freedesktop.org/monado/monado/merge_requests/2219))
  - d/remote: Add and use a multi-os `r_socket_t` typedef, with `R_SOCKET_FMT` to
    define the printf format to use for a socket descriptor
    ([!2165](https://gitlab.freedesktop.org/monado/monado/merge_requests/2165))
  - d/vive: Use raw imu samples for SLAM
    ([!2131](https://gitlab.freedesktop.org/monado/monado/merge_requests/2131))
  - d/wmr: Properly compute hand tracking boundary circle
    ([!2131](https://gitlab.freedesktop.org/monado/monado/merge_requests/2131),
    [!2131](https://gitlab.freedesktop.org/monado/monado/merge_requests/2131),
    [!2173](https://gitlab.freedesktop.org/monado/monado/merge_requests/2173))
  - d/xreal_air,d/vive: Reduce relation history lock contention
    ([!1949](https://gitlab.freedesktop.org/monado/monado/merge_requests/1949))
  - d/{wmr,rift_s,vive,ns}: Share hand bounding box with head tracker
    ([!2131](https://gitlab.freedesktop.org/monado/monado/merge_requests/2131),
    [!2143](https://gitlab.freedesktop.org/monado/monado/merge_requests/2143))
  - depthai: Add a new frameserver driver that supports some of the DepthAI
    cameras.
    ([!836](https://gitlab.freedesktop.org/monado/monado/merge_requests/836),
    [!831](https://gitlab.freedesktop.org/monado/monado/merge_requests/831),
    [!837](https://gitlab.freedesktop.org/monado/monado/merge_requests/837),
    [!858](https://gitlab.freedesktop.org/monado/monado/merge_requests/858),
    [!934](https://gitlab.freedesktop.org/monado/monado/merge_requests/934),
    [!1027](https://gitlab.freedesktop.org/monado/monado/merge_requests/1027),
    [!1029](https://gitlab.freedesktop.org/monado/monado/merge_requests/1029),
    [!1063](https://gitlab.freedesktop.org/monado/monado/merge_requests/1063),
    [!1097](https://gitlab.freedesktop.org/monado/monado/merge_requests/1097),
    [!1153](https://gitlab.freedesktop.org/monado/monado/merge_requests/1153),
    [!1260](https://gitlab.freedesktop.org/monado/monado/merge_requests/1260),
    [!1278](https://gitlab.freedesktop.org/monado/monado/merge_requests/1278),
    [!1282](https://gitlab.freedesktop.org/monado/monado/merge_requests/1282),
    [!1360](https://gitlab.freedesktop.org/monado/monado/merge_requests/1360),
    [!1494](https://gitlab.freedesktop.org/monado/monado/merge_requests/1494),
    [!1519](https://gitlab.freedesktop.org/monado/monado/merge_requests/1519),
    [!1523](https://gitlab.freedesktop.org/monado/monado/merge_requests/1523),
    [!1603](https://gitlab.freedesktop.org/monado/monado/merge_requests/1603),
    [!1649](https://gitlab.freedesktop.org/monado/monado/merge_requests/1649),
    [!1692](https://gitlab.freedesktop.org/monado/monado/merge_requests/1692),
    [!1769](https://gitlab.freedesktop.org/monado/monado/merge_requests/1769),
    [!1770](https://gitlab.freedesktop.org/monado/monado/merge_requests/1770),
    [!1832](https://gitlab.freedesktop.org/monado/monado/merge_requests/1832),
    [!1839](https://gitlab.freedesktop.org/monado/monado/merge_requests/1839),
    [!1881](https://gitlab.freedesktop.org/monado/monado/merge_requests/1881),
    [!2228](https://gitlab.freedesktop.org/monado/monado/merge_requests/2228))
  - euroc: Add euroc driver that plays EuRoC datasets for SLAM system evaluation.
    ([!880](https://gitlab.freedesktop.org/monado/monado/merge_requests/880))
  - ht: Error out if we can't find a hand-tracking model directory.
    ([!1831](https://gitlab.freedesktop.org/monado/monado/merge_requests/1831))
  - multi: Enable specifying arbitrary xrt_input_name for querying tracker poses.
    ([!741](https://gitlab.freedesktop.org/monado/monado/merge_requests/741))
  - north_star: Upstream Moshi Turner's "VIPD" distortion.
    ([!839](https://gitlab.freedesktop.org/monado/monado/merge_requests/839))
  - north_star: Fix the FOV calc on the v1/3D distortion.
    ([!839](https://gitlab.freedesktop.org/monado/monado/merge_requests/839))
  - north_star: General improvement of code organization.
    ([!839](https://gitlab.freedesktop.org/monado/monado/merge_requests/839))
  - north_star: Improved JSON parsing.
    ([!839](https://gitlab.freedesktop.org/monado/monado/merge_requests/839))
  - ohmd: Support OpenHMD controllers and specifically the Oculus Touch controller.
    ([!742](https://gitlab.freedesktop.org/monado/monado/merge_requests/742))
  - ohmd: Fix warnings and tidy.
    ([!2038](https://gitlab.freedesktop.org/monado/monado/merge_requests/2038))
  - ohmd: Disable WMR and Rift S drivers with a warning to use the native
    drivers.
    ([!2221](https://gitlab.freedesktop.org/monado/monado/merge_requests/2221))
  - opengloves: Refactor creation.
    ([!1987](https://gitlab.freedesktop.org/monado/monado/merge_requests/1987))
  - pssense: Add trigger force feedback.
    ([!1916](https://gitlab.freedesktop.org/monado/monado/merge_requests/1916))
  - psvr: Ensure that timestamps are always monotonic, stopping any time-traveling
    sample packets.
    ([!717](https://gitlab.freedesktop.org/monado/monado/merge_requests/717))
  - qwerty: Add qwerty driver for emulating headset and controllers with mouse and
    keyboard.
    ([!714](https://gitlab.freedesktop.org/monado/monado/merge_requests/714),
    [!1789](https://gitlab.freedesktop.org/monado/monado/merge_requests/1789),
    [!1926](https://gitlab.freedesktop.org/monado/monado/merge_requests/1926))
  - qwerty: Fix input timestamps for select and menu.
    ([!2080](https://gitlab.freedesktop.org/monado/monado/merge_requests/2080))
  - realsense: Expand driver to support non-T26x cameras and external SLAM
    tracking.
    ([!907](https://gitlab.freedesktop.org/monado/monado/merge_requests/907))
  - remote: Greatly improve the remote driver. Properly shut down the main loop.
    Use the new `xrt_system_devices` as base class for `r_hub`. Expose the Valve
    Index Controller instead of the simple controller as it better allows to map
    other controllers to it. Reuse the vive bindings helper library.
    ([!1356](https://gitlab.freedesktop.org/monado/monado/merge_requests/1356))
  - remote: Fix warnings found with GCC 13.
    ([!1921](https://gitlab.freedesktop.org/monado/monado/merge_requests/1921))
  - remote: Add support for new dynamic device roles.
    ([!1992](https://gitlab.freedesktop.org/monado/monado/merge_requests/1992))
  - remote: Add support for local_floor space.
    ([!2018](https://gitlab.freedesktop.org/monado/monado/merge_requests/2018))
  - remote: Fix socket closing on Windows by using socket_close.
    ([!2060](https://gitlab.freedesktop.org/monado/monado/merge_requests/2060),
    [!2061](https://gitlab.freedesktop.org/monado/monado/merge_requests/2061))
  - rift_s: Add Rift-S driver, this works with Monado's hand and SLAM tracking.
    ([!1447](https://gitlab.freedesktop.org/monado/monado/merge_requests/1447),
    [!1580](https://gitlab.freedesktop.org/monado/monado/merge_requests/1580),
    [!1665](https://gitlab.freedesktop.org/monado/monado/merge_requests/1665),
    [!1691](https://gitlab.freedesktop.org/monado/monado/merge_requests/1691),
    [!1823](https://gitlab.freedesktop.org/monado/monado/merge_requests/1823))
  - rs: Fix warnings with function declarations.
    ([!1989](https://gitlab.freedesktop.org/monado/monado/merge_requests/1989))
  - simulated: Support reference space usage via debug printing.
    ([!2091](https://gitlab.freedesktop.org/monado/monado/merge_requests/2091))
  - steamvr_lh: Add driver that wraps the SteamVR Lighthouse driver.
    ([!1861](https://gitlab.freedesktop.org/monado/monado/merge_requests/1861),
    [!1927](https://gitlab.freedesktop.org/monado/monado/merge_requests/1927),
    [!1943](https://gitlab.freedesktop.org/monado/monado/merge_requests/1943),
    [!1947](https://gitlab.freedesktop.org/monado/monado/merge_requests/1947),
    [!1950](https://gitlab.freedesktop.org/monado/monado/merge_requests/1950),
    [!2077](https://gitlab.freedesktop.org/monado/monado/merge_requests/2077),
    [!2090](https://gitlab.freedesktop.org/monado/monado/merge_requests/2090))
  - steamvr_lh: Add Vive Pro support.
    ([!1927](https://gitlab.freedesktop.org/monado/monado/merge_requests/1927))
  - steamvr_lh: Add Index support, also support canting of views.
    ([!1927](https://gitlab.freedesktop.org/monado/monado/merge_requests/1927))
  - steamvr_lh: Add Valve Knuckles support, also support hand tracking.
    ([!1927](https://gitlab.freedesktop.org/monado/monado/merge_requests/1927))
  - steamvr_lh: Basic vive tracker support.
    ([!1927](https://gitlab.freedesktop.org/monado/monado/merge_requests/1927),
    [!1927](https://gitlab.freedesktop.org/monado/monado/merge_requests/1927),
    [!1943](https://gitlab.freedesktop.org/monado/monado/merge_requests/1943))
  - steamvr_lh: Fix warnings with logger defines.
    ([!1929](https://gitlab.freedesktop.org/monado/monado/merge_requests/1929))
  - steamvr_lh: Set driver IPD & brightness on HMD.
    ([!1929](https://gitlab.freedesktop.org/monado/monado/merge_requests/1929),
    [!1929](https://gitlab.freedesktop.org/monado/monado/merge_requests/1929),
    [!1943](https://gitlab.freedesktop.org/monado/monado/merge_requests/1943))
  - steamvr_lh: Fix prediction and jitter and remove old `LH_PREDICTION` env var.
    ([!1943](https://gitlab.freedesktop.org/monado/monado/merge_requests/1943))
  - steamvr_lh: Use proper timestamp on hands and fix angular/linear velocity
    handling.
    ([!1947](https://gitlab.freedesktop.org/monado/monado/merge_requests/1947))
  - steamvr_lh: Add a mutex to update_inputs() to prevent unsafe condition in
    lighthouse driver.
    ([!1968](https://gitlab.freedesktop.org/monado/monado/merge_requests/1968))
  - steamvr_lh: Add tundra as a generic tracker
    ([!1979](https://gitlab.freedesktop.org/monado/monado/merge_requests/1979))
  - steamvr_lh: Silence some useless logging and properly wait for vive wands to
    settle
    ([!1986](https://gitlab.freedesktop.org/monado/monado/merge_requests/1986))
  - steamvr_lh: Simplify coordinate space conversion.
    ([!2090](https://gitlab.freedesktop.org/monado/monado/merge_requests/2090))
  - steamvr_lh: Make playspace reading more robust by choosing the first tracking
    universe from `lighthousedb.json` that is found in `chaperone_info.vrchap`.
    ([!2114](https://gitlab.freedesktop.org/monado/monado/merge_requests/2114))
  - steamvr_lh: Add additional bindings for vive and index controllers.
    ([!2115](https://gitlab.freedesktop.org/monado/monado/merge_requests/2115))
  - steamvr_lh: Introduce new driver interface
    ([!2136](https://gitlab.freedesktop.org/monado/monado/merge_requests/2136),
    [!2213](https://gitlab.freedesktop.org/monado/monado/merge_requests/2213))
  - steamvr_lh: Adjust location flags behavior for better lighthouse tracking on
    occlusion
    ([!2212](https://gitlab.freedesktop.org/monado/monado/merge_requests/2212))
  - survive: Add support for Gen 3.0 and Tundra trackers.
    ([!1860](https://gitlab.freedesktop.org/monado/monado/merge_requests/1860))
  - survive: Add support for HTC Vive Pro 2
    ([!1911](https://gitlab.freedesktop.org/monado/monado/merge_requests/1911))
  - survive: Fall back to default ipd if survive reports 0.0
    ([!2128](https://gitlab.freedesktop.org/monado/monado/merge_requests/2128))
  - survive/vive: Use new common controller bindings in `a/vive`.
    ([!1265](https://gitlab.freedesktop.org/monado/monado/merge_requests/1265))
  - ulv5: Add orientation values to hand joints.
    ([!2118](https://gitlab.freedesktop.org/monado/monado/merge_requests/2118))
  - v4l2: Add tracing support.
    ([!858](https://gitlab.freedesktop.org/monado/monado/merge_requests/858))
  - vf: Show the time on the video test source video server.
    ([!715](https://gitlab.freedesktop.org/monado/monado/merge_requests/715))
  - vf: Some tidy, frame fixes and tracing support.
    ([!860](https://gitlab.freedesktop.org/monado/monado/merge_requests/860))
  - vive: Factor out json config parser and reuse it in survive driver.
    ([!674](https://gitlab.freedesktop.org/monado/monado/merge_requests/674))
  - vive: Add rotation pose prediction to HMD and Controllers
    ([!691](https://gitlab.freedesktop.org/monado/monado/merge_requests/691))
  - vive: Setup the variable tracking for imu fusion.
    ([!740](https://gitlab.freedesktop.org/monado/monado/merge_requests/740))
  - vive: Tidy code by improving comments, removing old print, and use defines for
    hardcoded values.
    ([!793](https://gitlab.freedesktop.org/monado/monado/merge_requests/793))
  - vive: Minor refactor to IMU conversion code, should be no functional change.
    ([!793](https://gitlab.freedesktop.org/monado/monado/merge_requests/793))
  - vive: Drian IMU packets at start, this helps reduce time drift due backed up
    packets confusing the timing code.
    ([!1829](https://gitlab.freedesktop.org/monado/monado/merge_requests/1829))
  - vive: Refactor timing code in source, make it take in account of the age of
    samples, this reduces the time drift due to irregular delivery of packets.
    ([!1829](https://gitlab.freedesktop.org/monado/monado/merge_requests/1829))
  - vive: Add support for Gen 3.0 and Tundra trackers.
    ([!1860](https://gitlab.freedesktop.org/monado/monado/merge_requests/1860),
    [!1860](https://gitlab.freedesktop.org/monado/monado/merge_requests/1860),
    [!1863](https://gitlab.freedesktop.org/monado/monado/merge_requests/1863))
  - vive: Try to set realtime priority on sensors thread
    ([!1881](https://gitlab.freedesktop.org/monado/monado/merge_requests/1881))
  - vive: Set the correct tracking origin type when we have SLAM.
    ([!1893](https://gitlab.freedesktop.org/monado/monado/merge_requests/1893))
  - vive: Add support for HTC Vive Pro 2
    ([!1911](https://gitlab.freedesktop.org/monado/monado/merge_requests/1911))
  - vive: Follow common naming of update_inputs.
    ([!2038](https://gitlab.freedesktop.org/monado/monado/merge_requests/2038))
  - wmr: Add Windows Mixed Reality driver, supports 6dof through Basalt.
    ([!774](https://gitlab.freedesktop.org/monado/monado/merge_requests/774),
    [!803](https://gitlab.freedesktop.org/monado/monado/merge_requests/803),
    [!1796](https://gitlab.freedesktop.org/monado/monado/merge_requests/1796),
    [!1797](https://gitlab.freedesktop.org/monado/monado/merge_requests/1797))
  - wmr: Add SLAM (6dof inside-out) tracking for WMR headsets.
    ([!1035](https://gitlab.freedesktop.org/monado/monado/merge_requests/1035))
  - wmr: Add initial hand tracking for WMR devices.
    ([!1264](https://gitlab.freedesktop.org/monado/monado/merge_requests/1264))
  - wmr: Add auto exposure and gain module.
    ([!1291](https://gitlab.freedesktop.org/monado/monado/merge_requests/1291))
  - wmr: Send calibration automatically to SLAM tracker. This makes WMR SLAM
    tracking work out of the box without user intervention with Basalt.
    ([!1334](https://gitlab.freedesktop.org/monado/monado/merge_requests/1334))
  - wmr: Move driver over to builder interface. Currently only a simpler builder,
    the SLAM and Hand-Tracking setup hasn't been moved out yet.
    ([!1754](https://gitlab.freedesktop.org/monado/monado/merge_requests/1754))
  - wmr: Improve WMR controller orientation when in 3DoF by using the information
    that is available in the JSON config that is stored on the controllers.
    ([!1858](https://gitlab.freedesktop.org/monado/monado/merge_requests/1858))
  - wmr: Reduce drifting by applying calibration biases to controllers, doesn't
    fully eliminate as calibration is lacking for all temperature ranges.
    ([!1876](https://gitlab.freedesktop.org/monado/monado/merge_requests/1876))
  - wmr: Try to set realtime priority on USB thread
    ([!1881](https://gitlab.freedesktop.org/monado/monado/merge_requests/1881))
  - wmr: Add `WMR_LEFT_DISPLAY_VIEW_Y_OFFSET` and `WMR_RIGHT_DISPLAY_VIEW_Y_OFFSET`
    environmental variables to adjust screen distortion.
    ([!1988](https://gitlab.freedesktop.org/monado/monado/merge_requests/1988))
  - wmr: Add Dell Visor support to WMR driver.
    ([!2023](https://gitlab.freedesktop.org/monado/monado/merge_requests/2023))
  - wmr: Follow common naming of update_inputs.
    ([!2038](https://gitlab.freedesktop.org/monado/monado/merge_requests/2038))
  - wmr: Add support for Acer AH101 HMD
    ([!2222](https://gitlab.freedesktop.org/monado/monado/merge_requests/2222))
- IPC
  - Add support for thread hint function.
    ([!1951](https://gitlab.freedesktop.org/monado/monado/merge_requests/1951))
  - Android: Do not require OPENXR permission when connecting to MonadoService.
    Permission will not be granted if install application before permission
    container.
    ([!1213](https://gitlab.freedesktop.org/monado/monado/merge_requests/1213))
  - all: Use `libbsd` pidfile to detect running Monado instances. Enables
    automatically deleting stale socket files. The socket file is now placed in
    `$XDG_RUNTIME_DIR/monado_comp_ipc` by default or falls back to
    `/tmp/monado_comp_ipc` again if `$XDG_RUNTIME_DIR` is not set.
    ([!565](https://gitlab.freedesktop.org/monado/monado/merge_requests/565))
  - all: Transfer HMD blend mode, don't drop it on the floor.
    ([!694](https://gitlab.freedesktop.org/monado/monado/merge_requests/694))
  - all: Now that there is a interface that allows the compositor to support
    multi-client rendering use that instead of doing our own rendering.
    ([!721](https://gitlab.freedesktop.org/monado/monado/merge_requests/721),
    [!754](https://gitlab.freedesktop.org/monado/monado/merge_requests/754),
    [!768](https://gitlab.freedesktop.org/monado/monado/merge_requests/768),
    [!800](https://gitlab.freedesktop.org/monado/monado/merge_requests/800),
    [!846](https://gitlab.freedesktop.org/monado/monado/merge_requests/846))
  - all: Ensure that functions that require the compositor can't be called if a
    session has not been created yet.
    ([!768](https://gitlab.freedesktop.org/monado/monado/merge_requests/768))
  - all: Add Windows support to the IPC layer, this is based on named pipes.
    ([!1525](https://gitlab.freedesktop.org/monado/monado/merge_requests/1525),
    [!1531](https://gitlab.freedesktop.org/monado/monado/merge_requests/1531),
    [!1584](https://gitlab.freedesktop.org/monado/monado/merge_requests/1584),
    [!1807](https://gitlab.freedesktop.org/monado/monado/merge_requests/1807))
  - all: Add support for `XR_EXT_hand_interaction` profile - plumbs extension
    enabled state to ipc server/drivers.
    ([!1901](https://gitlab.freedesktop.org/monado/monado/merge_requests/1901))
  - all: Rename client connected function and document code.
    ([!1909](https://gitlab.freedesktop.org/monado/monado/merge_requests/1909))
  - all: Send less information when describing the client.
    ([!1909](https://gitlab.freedesktop.org/monado/monado/merge_requests/1909))
  - all: Add a stable ID for clients.
    ([!1909](https://gitlab.freedesktop.org/monado/monado/merge_requests/1909))
  - all: Implement performance settings interface, used to implement
    `XR_EXT_performance_settings`.
    ([!1936](https://gitlab.freedesktop.org/monado/monado/merge_requests/1936))
  - all: Add support for dynamic device roles.
    ([!1992](https://gitlab.freedesktop.org/monado/monado/merge_requests/1992),
    [!2013](https://gitlab.freedesktop.org/monado/monado/merge_requests/2013))
  - all: Add ability to do more complex IPC communication by introducing VLA
    functions. These lets us do the marshalling to some extent oursevles, useful
    for sending a non-fixed amount of data. This support is bi-directional.
    ([!2009](https://gitlab.freedesktop.org/monado/monado/merge_requests/2009),
    [!2009](https://gitlab.freedesktop.org/monado/monado/merge_requests/2009),
    [!2053](https://gitlab.freedesktop.org/monado/monado/merge_requests/2053))
  - all: Add variable length get views function.
    ([!2009](https://gitlab.freedesktop.org/monado/monado/merge_requests/2009))
  - all: Add support for getting the device visibility mask.
    ([!2016](https://gitlab.freedesktop.org/monado/monado/merge_requests/2016),
    [!2032](https://gitlab.freedesktop.org/monado/monado/merge_requests/2032))
  - all: Share per client thread shutdown code between platforms.
    ([!2046](https://gitlab.freedesktop.org/monado/monado/merge_requests/2046))
  - all: Add support for reference space usage.
    ([!2048](https://gitlab.freedesktop.org/monado/monado/merge_requests/2048))
  - all: Implement display refresh rate functions.
    ([!2051](https://gitlab.freedesktop.org/monado/monado/merge_requests/2051),
    [!2054](https://gitlab.freedesktop.org/monado/monado/merge_requests/2054))
  - all: Add support for recentering local spaces.
    ([!2055](https://gitlab.freedesktop.org/monado/monado/merge_requests/2055))
  - all: Add support for the new `xrt_system` and `xrt_session` objects.
    ([!2062](https://gitlab.freedesktop.org/monado/monado/merge_requests/2062),
    [!2079](https://gitlab.freedesktop.org/monado/monado/merge_requests/2079),
    [!2095](https://gitlab.freedesktop.org/monado/monado/merge_requests/2095))
  - all: Add interface for XR_FB_passthrough in Ipc communication.
    ([!2124](https://gitlab.freedesktop.org/monado/monado/merge_requests/2124))
  - all: Forwards the results of server swapchain image waits to
    xrWaitSwapchainImage client calls
    ([!2133](https://gitlab.freedesktop.org/monado/monado/merge_requests/2133))
  - all: Add support for get bounds rect function, used to implement
    `xrGetReferenceSpaceBoundsRect`.
    ([!2180](https://gitlab.freedesktop.org/monado/monado/merge_requests/2180))
  - android: Dup the fd from JVM and maintain it in native.
    ([!1924](https://gitlab.freedesktop.org/monado/monado/merge_requests/1924))
  - client: Refactor out the connection connect code into a its own file, this lets
    it be reused by other things that might want to connect like monado-ctl and
    libmonado.
    ([!1875](https://gitlab.freedesktop.org/monado/monado/merge_requests/1875))
  - client: Destroy the shared memory area when shutting down.
    ([!1906](https://gitlab.freedesktop.org/monado/monado/merge_requests/1906))
  - client: Add a interface header for `ipc_instance_create`.
    ([!1917](https://gitlab.freedesktop.org/monado/monado/merge_requests/1917))
  - client: Generate limited unique ids for native swapchains.
    ([!1957](https://gitlab.freedesktop.org/monado/monado/merge_requests/1957))
  - client: Refactor connection init code.
    ([!2011](https://gitlab.freedesktop.org/monado/monado/merge_requests/2011))
  - client: Check git tag as early as possible when connecting.
    ([!2011](https://gitlab.freedesktop.org/monado/monado/merge_requests/2011))
  - client: Use `log_level` from the message channel.
    ([!2022](https://gitlab.freedesktop.org/monado/monado/merge_requests/2022))
  - client: Add and use return check helpers, this makes it easier to see where the
    error happened.
    ([!2025](https://gitlab.freedesktop.org/monado/monado/merge_requests/2025),
    [!2025](https://gitlab.freedesktop.org/monado/monado/merge_requests/2025),
    [!2028](https://gitlab.freedesktop.org/monado/monado/merge_requests/2028))
  - client,server: Setting timer resolution (timeBeginPeriod) improves performance
    with NVIDIA drivers
    ([!2133](https://gitlab.freedesktop.org/monado/monado/merge_requests/2133))
  - server: Factor out the IPC server mainloop into a per-platform structure.
    ([!685](https://gitlab.freedesktop.org/monado/monado/merge_requests/685))
  - server: Destroy the shared memory area when shutting down.
    ([!1906](https://gitlab.freedesktop.org/monado/monado/merge_requests/1906))
  - server: Add a interface header for `ipc_server_main[_android]`.
    ([!1917](https://gitlab.freedesktop.org/monado/monado/merge_requests/1917))
  - server: Fix session deactivation negative array index access.
    ([!1991](https://gitlab.freedesktop.org/monado/monado/merge_requests/1991))
  - server: Use macro TEMP_FAILURE_RETRY to avoid closing a client connection on
    `-ENTR`.
    ([!2007](https://gitlab.freedesktop.org/monado/monado/merge_requests/2007),
    [!2012](https://gitlab.freedesktop.org/monado/monado/merge_requests/2012))
  - server: Read the exact command size in the client loop on Linux.
    ([!2053](https://gitlab.freedesktop.org/monado/monado/merge_requests/2053))
  - server: Make the server a little bit more chatty by default (switch the default
    logging level `info` from `warn`). Print out a message that the service has
    started, and advise how to collect information to help in debugging to ease
    helping end-users.
    ([!2094](https://gitlab.freedesktop.org/monado/monado/merge_requests/2094))
  - server: Don't call teardown if mutex fails to be created.
    ([!2095](https://gitlab.freedesktop.org/monado/monado/merge_requests/2095))
  - server: Pass in log_level to init function.
    ([!2095](https://gitlab.freedesktop.org/monado/monado/merge_requests/2095))
  - server: Use correct log define error message.
    ([!2095](https://gitlab.freedesktop.org/monado/monado/merge_requests/2095))
  - server: Print more client info.
    ([!2095](https://gitlab.freedesktop.org/monado/monado/merge_requests/2095))
  - server: Re-order functions [NFC]
    ([!2096](https://gitlab.freedesktop.org/monado/monado/merge_requests/2096))
  - server: Add "XRT_NO_STDIN" option disables stdin and prevents monado-service
    from terminating.
    ([!2133](https://gitlab.freedesktop.org/monado/monado/merge_requests/2133))
  - shared: Change IPC script to automatically mark all input aggregates as const.
    ([!1408](https://gitlab.freedesktop.org/monado/monado/merge_requests/1408))
  - shared: Add function to unmap the shared memory area when destroying.
    ([!1906](https://gitlab.freedesktop.org/monado/monado/merge_requests/1906))
  - shared: Minor tidy of various shared files.
    ([!2022](https://gitlab.freedesktop.org/monado/monado/merge_requests/2022))
  - shared: Break out message channel functions to own files.
    ([!2022](https://gitlab.freedesktop.org/monado/monado/merge_requests/2022))
  - shared: Add `ipc_print_result` helper.
    ([!2025](https://gitlab.freedesktop.org/monado/monado/merge_requests/2025))
  - shared: Minor fixes and tidy ipc_generated_protocol.h file.
    ([!2030](https://gitlab.freedesktop.org/monado/monado/merge_requests/2030))
  - shared: Add ipc_command_size function to the protocol generation.
    ([!2053](https://gitlab.freedesktop.org/monado/monado/merge_requests/2053))
- Compositor
  - Android: Refactor surface creation flow.
    ([!1742](https://gitlab.freedesktop.org/monado/monado/merge_requests/1742))
  - all: Rename all `num_` parameters and fields, typically to `_count`, to match
    OpenXR convention.
    ([!977](https://gitlab.freedesktop.org/monado/monado/merge_requests/977))
  - android: Default to compute compositor to work around issue
    [381](https://gitlab.freedesktop.org/monado/monado/-/issues/381).
    ([!2236](https://gitlab.freedesktop.org/monado/monado/merge_requests/2236))
  - client: Handle EGL_NO_CONTEXT_KHR gracefully if the EGLDisplay supports
    EGL_KHR_no_config_context.
    ([!705](https://gitlab.freedesktop.org/monado/monado/merge_requests/705))
  - client: Use the EGL compositor's display in swapchain, previously it tried to
    use the current one, which when running on a new thread would explode.
    ([!827](https://gitlab.freedesktop.org/monado/monado/merge_requests/827))
  - client: Initial support for D3D11 client applications on Windows.
    ([!943](https://gitlab.freedesktop.org/monado/monado/merge_requests/943),
    [!1263](https://gitlab.freedesktop.org/monado/monado/merge_requests/1263),
    [!1295](https://gitlab.freedesktop.org/monado/monado/merge_requests/1295),
    [!1326](https://gitlab.freedesktop.org/monado/monado/merge_requests/1326),
    [!1302](https://gitlab.freedesktop.org/monado/monado/merge_requests/1302),
    [!1337](https://gitlab.freedesktop.org/monado/monado/merge_requests/1337),
    [!1340](https://gitlab.freedesktop.org/monado/monado/merge_requests/1340))
  - client: Wait on Vulkan clients to complete rendering.
    ([!1117](https://gitlab.freedesktop.org/monado/monado/merge_requests/1117))
  - client: Set default log level on vk_bundle in Vulkan compositor.
    ([!1142](https://gitlab.freedesktop.org/monado/monado/merge_requests/1142))
  - client: Fence the client work and send fence to the native compositor.
    ([!1142](https://gitlab.freedesktop.org/monado/monado/merge_requests/1142))
  - client: Initial support for D3D12 client applications on Windows.
    ([!1340](https://gitlab.freedesktop.org/monado/monado/merge_requests/1340))
  - client: Support for OpenGL client applications on Windows.
    ([!1465](https://gitlab.freedesktop.org/monado/monado/merge_requests/1465))
  - client: Reduce the minimum required OpenGL version for client applications to
    3.0.
    ([!1465](https://gitlab.freedesktop.org/monado/monado/merge_requests/1465),
    [#47](https://gitlab.freedesktop.org/monado/monado/issues/47))
  - client: Do not use the global command buffer pool in the Vulkan compositor.
    ([!1748](https://gitlab.freedesktop.org/monado/monado/merge_requests/1748))
  - client: Silence VK_FORMAT_R32_SFLOAT warning in OpenGL code.
    ([!1750](https://gitlab.freedesktop.org/monado/monado/merge_requests/1750))
  - client: Don't use vkDeviceWaitIdle, because it requires all queues to be
    externally synchronized which we can't enforce.
    ([!1751](https://gitlab.freedesktop.org/monado/monado/merge_requests/1751))
  - client: Use correct format in get_swapchain_create_properties functions, client
    compositors are given their formats, make then translate to Vulkan before
    passing on.
    ([!1763](https://gitlab.freedesktop.org/monado/monado/merge_requests/1763))
  - client: Respect native compositor's extra usage bits, so we can remove the
    hardcoded always sampled bit. This also ensures that images have exactly the
    same usages in both the compositor and app.
    ([!1763](https://gitlab.freedesktop.org/monado/monado/merge_requests/1763))
  - client: Wait till D3D12 images aren't in use before releasing the swapchain.
    ([!1868](https://gitlab.freedesktop.org/monado/monado/merge_requests/1868))
  - client: Use D3D12 allocator, and work around NVIDIA bug.
    ([!1920](https://gitlab.freedesktop.org/monado/monado/merge_requests/1920))
  - client: Make sure to not double CloseHandle semaphore HANDLE.
    ([!1935](https://gitlab.freedesktop.org/monado/monado/merge_requests/1935))
  - client: Expose size limit for swapchains.
    ([!1964](https://gitlab.freedesktop.org/monado/monado/merge_requests/1964),
    [!2066](https://gitlab.freedesktop.org/monado/monado/merge_requests/2066))
  - client: Add and use helpers to unwrap native swapchains and compositors.
    ([!1982](https://gitlab.freedesktop.org/monado/monado/merge_requests/1982))
  - client: Make it possible to set log level in Vulkan compositor.
    ([!1993](https://gitlab.freedesktop.org/monado/monado/merge_requests/1993))
  - client: Add renderdoc_enabled implementation for Vulkan/OpenGL only on Android
    platform
    ([!2005](https://gitlab.freedesktop.org/monado/monado/merge_requests/2005))
  - client: Remove event functions.
    ([!2062](https://gitlab.freedesktop.org/monado/monado/merge_requests/2062))
  - client: Replace `glTextureStorageMem2DEXT` with `glTexStorageMem2DEXT` to adapt
    to more vendors' GPU drivers.
    ([!2117](https://gitlab.freedesktop.org/monado/monado/merge_requests/2117))
  - client: Add interface for XR_FB_passthrough in client side.
    ([!2124](https://gitlab.freedesktop.org/monado/monado/merge_requests/2124))
  - client: Run D3D12 swapchain initial barriers after all possible points of
    failure.
    ([!2161](https://gitlab.freedesktop.org/monado/monado/merge_requests/2161))
  - client/util: Fix several flags being set wrong on barriers and creation of the
    swapchain images. We were especially wrong with the depth stencil formats.
    ([!1119](https://gitlab.freedesktop.org/monado/monado/merge_requests/1119),
    [!1124](https://gitlab.freedesktop.org/monado/monado/merge_requests/1124),
    [!1125](https://gitlab.freedesktop.org/monado/monado/merge_requests/1125),
    [!1128](https://gitlab.freedesktop.org/monado/monado/merge_requests/1128))
  - comp: Fix layer submission on NVIDIA Tegra.
    ([!677](https://gitlab.freedesktop.org/monado/monado/merge_requests/677))
  - main: Integrate new frame timing code.
    ([!697](https://gitlab.freedesktop.org/monado/monado/merge_requests/697))
  - main: Make it possible to create the swapchain later when actually needed,
    and have the swapchain be in a non-ready state that stops drawing.
    ([!767](https://gitlab.freedesktop.org/monado/monado/merge_requests/767),
    [#120](https://gitlab.freedesktop.org/monado/monado/issues/120),
    [!787](https://gitlab.freedesktop.org/monado/monado/merge_requests/787))
  - main: Do not list VK_FORMAT_A2B10G10R10_UNORM_PACK32 as a supported format,
    it's not enough to show linear colours without banding but isn't used that
    often so do not list it.
    ([!833](https://gitlab.freedesktop.org/monado/monado/merge_requests/833))
  - main: Also resize on VK_SUBOPTIMAL_KHR.
    ([!841](https://gitlab.freedesktop.org/monado/monado/merge_requests/841))
  - main: Increase the usage of the `get_vk` helper function.
    ([!967](https://gitlab.freedesktop.org/monado/monado/merge_requests/967))
  - main: Use the new helpers to reduce code in main library.
    ([!967](https://gitlab.freedesktop.org/monado/monado/merge_requests/967))
  - main: Add support for mirroring the left view back to the debug gui, so we can
    record it or see what somebody's doing in VR.
    ([!1120](https://gitlab.freedesktop.org/monado/monado/merge_requests/1120),
    [!1135](https://gitlab.freedesktop.org/monado/monado/merge_requests/1135),
    [!1144](https://gitlab.freedesktop.org/monado/monado/merge_requests/1144))
  - main: Use at least 3 Vulkan images for comp_target_swapchain if supported.
    ([!1268](https://gitlab.freedesktop.org/monado/monado/merge_requests/1268))
  - main: Setting logging level when checking vulkan caps.
    ([!1268](https://gitlab.freedesktop.org/monado/monado/merge_requests/1268))
  - main: Refactor comp_target_swapchain to not pre-declare internal functions, we
    seem to be moving away from this style in the compositor so refactor the
    `comp_target_swapchain` file before adding the vblank thread in there.
    ([!1269](https://gitlab.freedesktop.org/monado/monado/merge_requests/1269))
  - main: Make `VK_KHR_external_[fence|semaphore]_fd` optional, this is helpful for
    CI where only lavapipe can be used which does not support those extensions.
    ([!1270](https://gitlab.freedesktop.org/monado/monado/merge_requests/1270))
  - main: Add thread waiting for vblank events, lets the fake pacer properly
    synchronise with hardware.
    ([!1271](https://gitlab.freedesktop.org/monado/monado/merge_requests/1271))
  - main: Init comp_base as early as possible, because it needs to be finalised
    last in destroy. It's basically a base class and should follow those semantics.
    ([!1316](https://gitlab.freedesktop.org/monado/monado/merge_requests/1316))
  - main: Propagate more errors from the renderer frame drawing and helper mirror
    functions.
    ([!1417](https://gitlab.freedesktop.org/monado/monado/merge_requests/1417))
  - main: Render cube layer
    ([!1421](https://gitlab.freedesktop.org/monado/monado/merge_requests/1421))
  - main: Introduce `comp_target_factory`. This struct allows us to remove long and
    cumbersome switch statements for each type. Instead the code is generic and
    tweaks for specific target types can be reused for others more easily with this
    data driven design of the code.
    ([!1570](https://gitlab.freedesktop.org/monado/monado/merge_requests/1570),
    [!1684](https://gitlab.freedesktop.org/monado/monado/merge_requests/1684))
  - main: Refactor arguments to `comp_target_create_images`, introduces the struct
    `comp_target_create_images_info`.
    ([!1601](https://gitlab.freedesktop.org/monado/monado/merge_requests/1601))
  - main: Refactor how surface formats are handled, this lets the compositor select
    which formats are considered exactly, and not just prefer one format.
    ([!1601](https://gitlab.freedesktop.org/monado/monado/merge_requests/1601))
  - main: Do not use the global command buffer pool.
    ([!1748](https://gitlab.freedesktop.org/monado/monado/merge_requests/1748))
  - main: Refactor to use vk_surface_info helper.
    ([!1801](https://gitlab.freedesktop.org/monado/monado/merge_requests/1801))
  - main: Refactor frame handling, makes semantics clearer.
    ([!1801](https://gitlab.freedesktop.org/monado/monado/merge_requests/1801),
    [!1801](https://gitlab.freedesktop.org/monado/monado/merge_requests/1801),
    [!1820](https://gitlab.freedesktop.org/monado/monado/merge_requests/1820))
  - main: Avoid acquiring early if the target isn't ready.
    ([!1801](https://gitlab.freedesktop.org/monado/monado/merge_requests/1801))
  - main: Prefer to only have two swapchains, useful for direct mode rendering.
    ([!1801](https://gitlab.freedesktop.org/monado/monado/merge_requests/1801))
  - main: Try to detect when we miss frames even without frame timing information.
    ([!1801](https://gitlab.freedesktop.org/monado/monado/merge_requests/1801))
  - main: Refactor mirror to debug gui code and add support for compute queue.
    ([!1820](https://gitlab.freedesktop.org/monado/monado/merge_requests/1820))
  - main: Use the new samplers on render_resources, remove the layer renderer
    framebuffer's samplers.
    ([!1824](https://gitlab.freedesktop.org/monado/monado/merge_requests/1824))
  - main: Optionally enable VK_EXT_debug_marker extension on debug builds.
    ([!1877](https://gitlab.freedesktop.org/monado/monado/merge_requests/1877))
  - main: Name all fence objects using debug helper function.
    ([!1877](https://gitlab.freedesktop.org/monado/monado/merge_requests/1877))
  - main: Use vk_cmd_submit_locked in vk_helper to simply peek logic
    ([!1884](https://gitlab.freedesktop.org/monado/monado/merge_requests/1884))
  - main: Add NorthStar to listed displays
    ([!1893](https://gitlab.freedesktop.org/monado/monado/merge_requests/1893))
  - main: Only wait on the main queue when drawing the frame.
    ([!1893](https://gitlab.freedesktop.org/monado/monado/merge_requests/1893))
  - main: Use enumeration helpers in and refactor the NVIDIA direct target code.
    ([!1894](https://gitlab.freedesktop.org/monado/monado/merge_requests/1894),
    [!1894](https://gitlab.freedesktop.org/monado/monado/merge_requests/1894),
    [!1913](https://gitlab.freedesktop.org/monado/monado/merge_requests/1913))
  - main: Set sequence number correctly on readback frames.
    ([!1902](https://gitlab.freedesktop.org/monado/monado/merge_requests/1902))
  - main: Fix warnings found with GCC 13.
    ([!1921](https://gitlab.freedesktop.org/monado/monado/merge_requests/1921))
  - main: Use more enumeration helpers.
    ([!1940](https://gitlab.freedesktop.org/monado/monado/merge_requests/1940))
  - main: Free plane_properties earlier.
    ([!1940](https://gitlab.freedesktop.org/monado/monado/merge_requests/1940))
  - main: Print creation info for direct mode objects.
    ([!1940](https://gitlab.freedesktop.org/monado/monado/merge_requests/1940))
  - main: Always use the mode's extents when creating the surface.
    ([!1940](https://gitlab.freedesktop.org/monado/monado/merge_requests/1940))
  - main: Use new layer squasher helpers and manage scratch images lifetime.
    ([!1955](https://gitlab.freedesktop.org/monado/monado/merge_requests/1955))
  - main: Trace mirror blit function.
    ([!1957](https://gitlab.freedesktop.org/monado/monado/merge_requests/1957))
  - main: Name the runtime Surface on Android.
    ([!1963](https://gitlab.freedesktop.org/monado/monado/merge_requests/1963))
  - main: Refactor the layer rendering code to use `render_gfx_render_pass`,
    `render_gfx_target_resources` and an `VkCommandBuffer` that is passed in as an
    argument to the draw call. This allows the layer renderer to share the scratch
    images with the compute pipeline.
    ([!1969](https://gitlab.freedesktop.org/monado/monado/merge_requests/1969),
    [!1969](https://gitlab.freedesktop.org/monado/monado/merge_requests/1969),
    [!1970](https://gitlab.freedesktop.org/monado/monado/merge_requests/1970))
  - main: Use VK_CHK_WITH_RET instead of vk_check_error, and convert a few other
    places to the helpers as well.
    ([!1971](https://gitlab.freedesktop.org/monado/monado/merge_requests/1971),
    [!2050](https://gitlab.freedesktop.org/monado/monado/merge_requests/2050))
  - main: Tidy headers in layer renderer.
    ([!1974](https://gitlab.freedesktop.org/monado/monado/merge_requests/1974))
  - main: Refactor the various getters of poses and view data so that they are
    shared between both graphics and compute paths.
    ([!1974](https://gitlab.freedesktop.org/monado/monado/merge_requests/1974))
  - main: Refactor graphics dispatch, this makes it easier to extract the code
    later down the line.
    ([!1974](https://gitlab.freedesktop.org/monado/monado/merge_requests/1974))
  - main: Wire up timewarp on the graphics path for the distortion shaders.
    ([!1981](https://gitlab.freedesktop.org/monado/monado/merge_requests/1981))
  - main: Use new graphics layer squasher.
    ([!1995](https://gitlab.freedesktop.org/monado/monado/merge_requests/1995))
  - main: Remove old layer renderer code and integration.
    ([!1995](https://gitlab.freedesktop.org/monado/monado/merge_requests/1995))
  - main: Add argument to specify display mode id for surface creation.
    ([!2010](https://gitlab.freedesktop.org/monado/monado/merge_requests/2010))
  - main: Fix multiple thread access to VkQueue in present.
    ([!2050](https://gitlab.freedesktop.org/monado/monado/merge_requests/2050))
  - main: Implement display refresh rates function stubs.
    ([!2051](https://gitlab.freedesktop.org/monado/monado/merge_requests/2051))
  - main: Remove events code, no longer needed.
    ([!2062](https://gitlab.freedesktop.org/monado/monado/merge_requests/2062))
  - main: Add enum to select FoV source, it was very unclear where exactly the FoV
    came from, this makes it clearer and also reduces the number of places it's
    accessed from.
    ([!2101](https://gitlab.freedesktop.org/monado/monado/merge_requests/2101))
  - main: Use new debuggable scratch images (one `comp_scratch_single_images` per
    view), used to drive the preview view in the UI and to debug the views.
    ([!2103](https://gitlab.freedesktop.org/monado/monado/merge_requests/2103))
  - main: Use vk_enumerate_swapchain_images helper.
    ([!2104](https://gitlab.freedesktop.org/monado/monado/merge_requests/2104))
  - main: Improve swapchain creation to print more debug information.
    ([!2104](https://gitlab.freedesktop.org/monado/monado/merge_requests/2104))
  - main: Split submit timing into begin and end.
    ([!2108](https://gitlab.freedesktop.org/monado/monado/merge_requests/2108))
  - main: Make sure to not use the array of displays if we fail to allocate it, and
    also tidy the code.
    ([!2113](https://gitlab.freedesktop.org/monado/monado/merge_requests/2113))
  - main: Let sub-classed targets override compositor extents. The big win here
    is that targets no longer writes the `preferred_[width|height]` on the
    compositor's settings struct. And this moves us closer to not using
    `comp_compositor` or `comp_settings` in the targets which means they can be
    refactored out of main and put into util, lettings us reuse them, and even
    have multiple targets active at the same time.
    ([!2113](https://gitlab.freedesktop.org/monado/monado/merge_requests/2113))
  - main: let compositor targets control more of vulkan initialization.
    ([!2134](https://gitlab.freedesktop.org/monado/monado/merge_requests/2134),
    [!2179](https://gitlab.freedesktop.org/monado/monado/merge_requests/2179))
  - main: Fix bug with incorrect surface format matching.
    ([!2183](https://gitlab.freedesktop.org/monado/monado/merge_requests/2183))
  - multi: Introduce a new multi client compositor layer, this allows rendering
    code
    to be moved from the IPC layer into the compositor, separating concerns. The
    main compositor always uses the multi client compositor, as it gives us a async
    render loop for free.
    ([!721](https://gitlab.freedesktop.org/monado/monado/merge_requests/721),
    [!754](https://gitlab.freedesktop.org/monado/monado/merge_requests/754),
    [!759](https://gitlab.freedesktop.org/monado/monado/merge_requests/759),
    [!1323](https://gitlab.freedesktop.org/monado/monado/merge_requests/1323),
    [!1346](https://gitlab.freedesktop.org/monado/monado/merge_requests/1346),
    [#171](https://gitlab.freedesktop.org/monado/monado/issues/171))
  - multi: Make sure there are at least some predicted data, to avoid asserts in
    non-service mode.
    ([!864](https://gitlab.freedesktop.org/monado/monado/merge_requests/864))
  - multi: Try to set realtime priority on main thread
    ([!1881](https://gitlab.freedesktop.org/monado/monado/merge_requests/1881))
  - multi: Add support for `XR_EXT_hand_interaction` profile.
    ([!1901](https://gitlab.freedesktop.org/monado/monado/merge_requests/1901))
  - multi: Add stub set thread hint function.
    ([!1951](https://gitlab.freedesktop.org/monado/monado/merge_requests/1951))
  - multi: Implement display refresh rate functions.
    ([!2051](https://gitlab.freedesktop.org/monado/monado/merge_requests/2051))
  - multi: Switch to use `xrt_session_event` and `xrt_session_event_sink`.
    ([!2062](https://gitlab.freedesktop.org/monado/monado/merge_requests/2062))
  - multi_layer_entry: Updated the array length of xscs within multi_layer_entry
    from 4 to `2 * XRT_MAX_VIEWS` to accommodate a variable number of views.
    ([!2086](https://gitlab.freedesktop.org/monado/monado/merge_requests/2086),
    [!2175](https://gitlab.freedesktop.org/monado/monado/merge_requests/2175),
    [!2189](https://gitlab.freedesktop.org/monado/monado/merge_requests/2189))
  - null: Add a new compositor intended to be used on CIs that use the Mesa
    software rasteriser vulkan driver. It is also intended to be a base for how to
    write a new compositor. It does no rendering and does not open up any window,
    so has less requirements then the main compositor, both in terms of CPU usage
    and build dependencies.
    ([!1319](https://gitlab.freedesktop.org/monado/monado/merge_requests/1319))
  - null: Remove events code, no longer needed.
    ([!2062](https://gitlab.freedesktop.org/monado/monado/merge_requests/2062))
  - render: Refactor and reorganize compositor to improve modularity and ease of
    reuse. This introduces the render folder which aims to be useful Vulkan render
    code that can be used outside of the compositor.
    ([!959](https://gitlab.freedesktop.org/monado/monado/merge_requests/959),
    [!959](https://gitlab.freedesktop.org/monado/monado/merge_requests/959),
    [!967](https://gitlab.freedesktop.org/monado/monado/merge_requests/967),
    [!970](https://gitlab.freedesktop.org/monado/monado/merge_requests/970),
    [!982](https://gitlab.freedesktop.org/monado/monado/merge_requests/982),
    [!1021](https://gitlab.freedesktop.org/monado/monado/merge_requests/1021))
  - render: Add fast path for single layer projection layer skipping the layer
    renderer and avoiding one copy.
    ([!959](https://gitlab.freedesktop.org/monado/monado/merge_requests/959))
  - render: Lots of refactoring and tidying in code, making it independent of the
    compositor and only depending on the vk_bundle.
    ([!959](https://gitlab.freedesktop.org/monado/monado/merge_requests/959))
  - render: Refactor out into own library.
    ([!967](https://gitlab.freedesktop.org/monado/monado/merge_requests/967))
  - render: Use query pool to measure GPU time.
    ([!1268](https://gitlab.freedesktop.org/monado/monado/merge_requests/1268))
  - render: Reuse a single command buffer instead of allocating/freeing it every
    frame.
    ([!1352](https://gitlab.freedesktop.org/monado/monado/merge_requests/1352))
  - render: Do not use the global command buffer pool, use `vk_cmd_pool` for
    distrion images upload.
    ([!1748](https://gitlab.freedesktop.org/monado/monado/merge_requests/1748))
  - render: Add new shared samplers, use them and remove the default sampler.
    ([!1824](https://gitlab.freedesktop.org/monado/monado/merge_requests/1824))
  - render: Various smaller commit to tidy the code,
    better documentation and naming of defines.
    ([!1955](https://gitlab.freedesktop.org/monado/monado/merge_requests/1955))
  - render: Refactor layer squasher code, the shader is now run once per view
    instead of doing two views in one submission. Makes it easier to split up
    targets and requires less samplers in one invocation.
    ([!1955](https://gitlab.freedesktop.org/monado/monado/merge_requests/1955))
  - render: Refactor scratch images so that they are fully their own struct and
    is managed by a user of the render code.
    ([!1955](https://gitlab.freedesktop.org/monado/monado/merge_requests/1955))
  - render: Optimize layer shader, cutting of around 5%-10% of execution time.
    ([!1955](https://gitlab.freedesktop.org/monado/monado/merge_requests/1955))
  - render: Stop timewarp stretching by changing math.
    ([!1956](https://gitlab.freedesktop.org/monado/monado/merge_requests/1956))
  - render: Don't enable depth testing and writing for mesh shader.
    ([!1969](https://gitlab.freedesktop.org/monado/monado/merge_requests/1969))
  - render: Refactor gfx path code to split out render pass vulkan objects from
    the render target resources struct into the `render_gfx_render_pass` struct.
    This allows the render pass to be reused for more than one target.
    ([!1969](https://gitlab.freedesktop.org/monado/monado/merge_requests/1969))
  - render: Use defines helpers from Vulkan helper code instead of defining self.
    ([!1972](https://gitlab.freedesktop.org/monado/monado/merge_requests/1972))
  - render: Refactor mesh distortion dispatch functions.
    ([!1974](https://gitlab.freedesktop.org/monado/monado/merge_requests/1974))
  - render: Expose render_calc_uv_to_tangent_lengths_rect function, document it
    better and also add tests for it.
    ([!1974](https://gitlab.freedesktop.org/monado/monado/merge_requests/1974))
  - render: Tweak cmake files so that comp_render is usable without any other of
    the compositor bits.
    ([!1974](https://gitlab.freedesktop.org/monado/monado/merge_requests/1974))
  - render: Add ability to sub-allocate UBOs from a larger buffer, both code and
    needed scaffolding to use it in the gfx path.
    ([!1976](https://gitlab.freedesktop.org/monado/monado/merge_requests/1976))
  - render: Make gfx mesh distortion shader sub-allocate it's UBO.
    ([!1976](https://gitlab.freedesktop.org/monado/monado/merge_requests/1976))
  - render: Refactor gfx mesh shader allocation and dispatch.
    ([!1980](https://gitlab.freedesktop.org/monado/monado/merge_requests/1980))
  - render: Remove unused render_gfx_view and other fields on render_gfx,
    the limiting factor to how many views the graphics path can do now is the sizes
    of descriptor pools and UBO buffer.
    ([!1980](https://gitlab.freedesktop.org/monado/monado/merge_requests/1980))
  - render: Refactor gfx descriptor pool, descriptor layout creation function,
    ubo upload ad descriptor updating function to be shareable. The common pattern
    is one UBO and one source image, so make it possible to share these.
    ([!1980](https://gitlab.freedesktop.org/monado/monado/merge_requests/1980))
  - render: Add timewarp to graphics path distortion shaders, works very similar to
    the compute paths timewarp.
    ([!1981](https://gitlab.freedesktop.org/monado/monado/merge_requests/1981))
  - render: Make it possible to set clear color when starting render pass.
    ([!1983](https://gitlab.freedesktop.org/monado/monado/merge_requests/1983))
  - render: Add new layer shaders and support code.
    ([!1983](https://gitlab.freedesktop.org/monado/monado/merge_requests/1983))
  - render: Prepare gfx shared one ubo and src code for addition of cylinder and
    equirect2 shaders.
    ([!1994](https://gitlab.freedesktop.org/monado/monado/merge_requests/1994))
  - render: Add cylinder and equirect2 shaders and code for graphics path.
    ([!1994](https://gitlab.freedesktop.org/monado/monado/merge_requests/1994))
  - render: Remove old graphics layer squasher.
    ([!1995](https://gitlab.freedesktop.org/monado/monado/merge_requests/1995))
  - render: `render_resources` now has a `view_count` field, which is set to 1 for
    mono and 2 for stereo. This is used to iterate over the views in the render
    function.
    ([!2086](https://gitlab.freedesktop.org/monado/monado/merge_requests/2086),
    [!2175](https://gitlab.freedesktop.org/monado/monado/merge_requests/2175),
    [!2189](https://gitlab.freedesktop.org/monado/monado/merge_requests/2189))
  - render: Use the `XRT_MAX_VIEWS` macro to calculate the length of a series of
    arrays.
    ([!2086](https://gitlab.freedesktop.org/monado/monado/merge_requests/2086),
    [!2175](https://gitlab.freedesktop.org/monado/monado/merge_requests/2175),
    [!2189](https://gitlab.freedesktop.org/monado/monado/merge_requests/2189))
  - render: Tweak alpha blending, before on the gfx path the written alpha was
    always zero, this would pose a problem when we want to display the scratch
    images in the debug UI as they would be completely black.
    ([!2103](https://gitlab.freedesktop.org/monado/monado/merge_requests/2103))
  - shaders: Add blit compute shader.
    ([!1820](https://gitlab.freedesktop.org/monado/monado/merge_requests/1820))
  - swapchain: Change `struct xrt_swapchain *l_xsc, struct xrt_swapchain *r_xsc` to
    `struct xrt_swapchain *xsc[XRT_MAX_VIEWS]`, in order to support multiple views'
    swapchains. When iterating, use `xrt_layer_data.view_count`.
    ([!2086](https://gitlab.freedesktop.org/monado/monado/merge_requests/2086),
    [!2175](https://gitlab.freedesktop.org/monado/monado/merge_requests/2175),
    [!2189](https://gitlab.freedesktop.org/monado/monado/merge_requests/2189))
  - util: Refactor swapchain and fence code to be more independent of compositor
    and put into own library. Joined by a `comp_base` helper that implements
    a lot of the more boilerplate compositor code.
    ([!967](https://gitlab.freedesktop.org/monado/monado/merge_requests/967))
  - util: Add Vulkan helper code to initialise a vk_bundle from scratch.
    ([!970](https://gitlab.freedesktop.org/monado/monado/merge_requests/970))
  - util: Completely propagate errors from image creation failures and some tidy.
    ([!1417](https://gitlab.freedesktop.org/monado/monado/merge_requests/1417),
    [!1417](https://gitlab.freedesktop.org/monado/monado/merge_requests/1417),
    [!2052](https://gitlab.freedesktop.org/monado/monado/merge_requests/2052))
  - util: Remove samplers from comp_swapchain_image, they were always the same.
    ([!1824](https://gitlab.freedesktop.org/monado/monado/merge_requests/1824))
  - util: Name all fence objects using debug helper function.
    ([!1877](https://gitlab.freedesktop.org/monado/monado/merge_requests/1877))
  - util: Improve Vulkan instance creation code to be clearer about what extensions
    are missing, also generally refactor function to make it better.
    ([!1885](https://gitlab.freedesktop.org/monado/monado/merge_requests/1885))
  - util: Make sure to not destroy invalid `VkSemaphore` objects.
    ([!1887](https://gitlab.freedesktop.org/monado/monado/merge_requests/1887))
  - util: Track native semaphore handles, following the semantics of other handles
    in Monado. This fixes the leak of `syncobj_file` on Linux.
    ([!1887](https://gitlab.freedesktop.org/monado/monado/merge_requests/1887))
  - util: Use enumeration helpers, with a tiny little bit of refactor to increase
    code reuse.
    ([!1894](https://gitlab.freedesktop.org/monado/monado/merge_requests/1894))
  - util: Add helpers to launch the compute layer squasher shaders and the compute
    distortion shaders. They are in `comp_util` because it looks at a list of
    `comp_layer` and `comp_swapchain` structs that `comp_base` manages.
    ([!1955](https://gitlab.freedesktop.org/monado/monado/merge_requests/1955),
    [!1955](https://gitlab.freedesktop.org/monado/monado/merge_requests/1955),
    [!1967](https://gitlab.freedesktop.org/monado/monado/merge_requests/1967),
    [!1975](https://gitlab.freedesktop.org/monado/monado/merge_requests/1975))
  - util: Generate limited limited ids for native swapchains.
    ([!1957](https://gitlab.freedesktop.org/monado/monado/merge_requests/1957))
  - util: Prefix compute functions with `cs`, rename file and refactor out layer
    helpers in preparation for new graphics layer render code.
    ([!1983](https://gitlab.freedesktop.org/monado/monado/merge_requests/1983))
  - util: Prepare code for addition of cylinder and equirect layers to graphics
    paths, like adding various helpers.
    ([!1994](https://gitlab.freedesktop.org/monado/monado/merge_requests/1994))
  - util: Add cylinder and equirect2 shaders and code for graphics path.
    ([!1994](https://gitlab.freedesktop.org/monado/monado/merge_requests/1994))
  - util: Replace is_view_index_visible helper by is_layer_view_visible.
    ([!1998](https://gitlab.freedesktop.org/monado/monado/merge_requests/1998))
  - util: Also clean up image views on mutex and cond variable creation error
    in the comp_swapchain.c file.
    ([!2052](https://gitlab.freedesktop.org/monado/monado/merge_requests/2052))
  - util: Refactor how arguments are given, this makes it easier to change the
    number of views that the code supports.
    ([!2101](https://gitlab.freedesktop.org/monado/monado/merge_requests/2101))
  - util: Add `comp_scratch_single_images` and `comp_scratch_stereo_images` helper
    struct, these uses `u_native_images_debug` this let us do zero copy viewing or
    debugging of the images.
    ([!2103](https://gitlab.freedesktop.org/monado/monado/merge_requests/2103))
  - util: Expand on swapchain import error codes. This allows the CTS in Direct3D
    12 to not fail when attempting to import sRGB swapchains with flags such as
    `XR_SWAPCHAIN_USAGE_UNORDERED_ACCESS_BIT`.
    ([!2167](https://gitlab.freedesktop.org/monado/monado/merge_requests/2167))
  - util: Fix double free when failing to initialize Vulkan swapchain
    util: Fix double free when failing to import non-Vulkan swapchain
    ([!2199](https://gitlab.freedesktop.org/monado/monado/merge_requests/2199))
  - util: Fix vk_deinit_mutex asserts when vk_create_device fails.
    ([!2214](https://gitlab.freedesktop.org/monado/monado/merge_requests/2214))
  - util: Fix a crash bug in `render_gfx_end_target` with non-compute pipeline path
    on certain Android devices when zero layers are committed.
    ([!2216](https://gitlab.freedesktop.org/monado/monado/merge_requests/2216))
  - xrt_layer_type: Renamed the `XRT_LAYER_STEREO_PROJECTION` to
    `XRT_LAYER_PROJECTION` and `XRT_LAYER_STEREO_PROJECTION_DEPTH` to
    `XRT_LAYER_PROJECTION_DEPTH` in the `xrt_layer_type` enumeration to support
    both mono and stereo projection layers. This change provides a more inclusive
    and versatile categorization of projection layers within the XRT framework,
    accommodating a wider range of use cases.
    ([!2086](https://gitlab.freedesktop.org/monado/monado/merge_requests/2086),
    [!2175](https://gitlab.freedesktop.org/monado/monado/merge_requests/2175),
    [!2189](https://gitlab.freedesktop.org/monado/monado/merge_requests/2189))
- Tracking
  - h/mercury: Push hand rect masks to the SLAM tracker sinks
    ([!2131](https://gitlab.freedesktop.org/monado/monado/merge_requests/2131))
  - hand: General tidy of the async code.
    ([!1893](https://gitlab.freedesktop.org/monado/monado/merge_requests/1893))
  - hand: Rename new user hand estimation switch.
    ([!1893](https://gitlab.freedesktop.org/monado/monado/merge_requests/1893))
  - hand: Add env variables to control prediction.
    ([!1893](https://gitlab.freedesktop.org/monado/monado/merge_requests/1893))
  - mercury: Add Levenberg-Marquardt optimizer and lots of improvements. Makes hand
    tracking finally somewhat usable.
    ([!1381](https://gitlab.freedesktop.org/monado/monado/merge_requests/1381))
  - t/hand: Reduce relation history lock contention
    ([!1949](https://gitlab.freedesktop.org/monado/monado/merge_requests/1949))
  - tracking: Remove unused destroy function on async interface.
    ([!1893](https://gitlab.freedesktop.org/monado/monado/merge_requests/1893))
- Helper Libraries
  - Introduce VIT loader to load a given VIT system, implement the VIT interface in
    SLAM tracker, and remove the unused MatFrame class. Only turn on the SLAM
    feature on Linux.
    ([!2058](https://gitlab.freedesktop.org/monado/monado/merge_requests/2058),
    [!2125](https://gitlab.freedesktop.org/monado/monado/merge_requests/2125),
    [!2144](https://gitlab.freedesktop.org/monado/monado/merge_requests/2144))
  - When a space is located in itself as base space, skip locating the space
    altogether.
    ([!2192](https://gitlab.freedesktop.org/monado/monado/merge_requests/2192))
  - a/bindings: Interaction profile inheritance, support data-inheritance in
    bindings.json, add a new concept of virtual profiles for profile like
    extensions (e.g. `XR_EXT_palm_pose`) which do not define a profile
    themselves but require their newly defined actions to be supported by all
    profiles.
    ([!1896](https://gitlab.freedesktop.org/monado/monado/merge_requests/1896))
  - a/bindings: Add support for `XR_EXT_hand_interaction` profile - Updates
    bindings & pretty-print for newly added support for `XR_EXT_hand_interaction`
    profile.
    ([!1901](https://gitlab.freedesktop.org/monado/monado/merge_requests/1901))
  - a/math: Fix const-correctness in m_relation_history
    ([!2133](https://gitlab.freedesktop.org/monado/monado/merge_requests/2133))
  - a/util: Fix crash bug with XR_EXT_dpad_binding after multiple session re-runs.
    ([!2133](https://gitlab.freedesktop.org/monado/monado/merge_requests/2133))
  - a/util: Fix missing lib in cmake file for building `mercury_steamvr_driver`
    ([!2169](https://gitlab.freedesktop.org/monado/monado/merge_requests/2169))
  - a/vive: Add FoV tweaks for another index HMD
    ([!1937](https://gitlab.freedesktop.org/monado/monado/merge_requests/1937))
  - all: Rename all `num_` parameters and fields, typically to `_count`, to match
    OpenXR convention.
    ([!977](https://gitlab.freedesktop.org/monado/monado/merge_requests/977))
  - android: Tidy code and add warning on not getting refresh rate.
    ([!1907](https://gitlab.freedesktop.org/monado/monado/merge_requests/1907))
  - android: Support creating surface with title.
    ([!1963](https://gitlab.freedesktop.org/monado/monado/merge_requests/1963))
  - android: Add argument to specify display mode id for surface creation.
    ([!2010](https://gitlab.freedesktop.org/monado/monado/merge_requests/2010))
  - aux/debug_ui: bump the minimum gl version to 4.5
    ([!2147](https://gitlab.freedesktop.org/monado/monado/merge_requests/2147))
  - bindings: Add support for eye gaze bindings extension.
    ([!1836](https://gitlab.freedesktop.org/monado/monado/merge_requests/1836))
  - bindings: Add generic vive tracker input and output bindings, not used for now.
    ([!1860](https://gitlab.freedesktop.org/monado/monado/merge_requests/1860))
  - bindings: Correct ML2 controller extension string.
    ([!1890](https://gitlab.freedesktop.org/monado/monado/merge_requests/1890))
  - bindings: Add system buttons to WinMR controllers, for OpenXR gate them behind
    the XR_MNDX_system_buttons extension.
    ([!1903](https://gitlab.freedesktop.org/monado/monado/merge_requests/1903))
  - bindings: Add OPPO MR controller profile.
    ([!1904](https://gitlab.freedesktop.org/monado/monado/merge_requests/1904))
  - bindings: Replaces the `monado_device` entry for `XR_EXT_hand_interaction` in
    bindings.json to refer to a new device name type.
    ([!1915](https://gitlab.freedesktop.org/monado/monado/merge_requests/1915))
  - cmake: Split the CMakeLists.txt out into the sub-directories of each library,
    making each much more manageable when editing.
    ([!1328](https://gitlab.freedesktop.org/monado/monado/merge_requests/1328))
  - d3d: Add D3D helpers used by various parts of Monado, mostly the D3D11 client
    compositor.
    ([!943](https://gitlab.freedesktop.org/monado/monado/merge_requests/943),
    [!1326](https://gitlab.freedesktop.org/monado/monado/merge_requests/1326),
    [!1302](https://gitlab.freedesktop.org/monado/monado/merge_requests/1302),
    [!1337](https://gitlab.freedesktop.org/monado/monado/merge_requests/1337))
  - d3d: Add a D3D12 allocator, certain use-cases in D3D12 requires the resource
    to be allocated directly in D3D12, like multi-gpu.
    ([!1920](https://gitlab.freedesktop.org/monado/monado/merge_requests/1920))
  - d3d: Add copy D3D12 helper functions, needed to work around issues with layout
    on small textures on NVIDIA hardware.
    ([!1920](https://gitlab.freedesktop.org/monado/monado/merge_requests/1920))
  - external/slam: Update to 7.0.0 with RESET_TRACKER_STATE and ignore masks
    ([!1937](https://gitlab.freedesktop.org/monado/monado/merge_requests/1937))
  - h/mercury: Add min detection confidence option
    ([!1937](https://gitlab.freedesktop.org/monado/monado/merge_requests/1937))
  - m/3dof: Add assert to catch time traveling drivers.
    ([!717](https://gitlab.freedesktop.org/monado/monado/merge_requests/717))
  - math: Fix for M_PI on Windows.
    ([!735](https://gitlab.freedesktop.org/monado/monado/merge_requests/735))
  - math: Add `math_map_ranges` function, does the same thing as Arduino's `map`.
    ([!839](https://gitlab.freedesktop.org/monado/monado/merge_requests/839))
  - math: Add clock_offset utility to estimate offset between clocks
    ([!1590](https://gitlab.freedesktop.org/monado/monado/merge_requests/1590))
  - math: Minor tidy of `m_api.h`, `m_base.cpp` and `CMakeLists.txt`.
    ([!1978](https://gitlab.freedesktop.org/monado/monado/merge_requests/1978))
  - math: Add function to calculate a vulkan infinite reverse projection matrix.
    ([!1978](https://gitlab.freedesktop.org/monado/monado/merge_requests/1978))
  - math: Refactor m_clock_offset_a2b to avoid precision problems.
    ([!2106](https://gitlab.freedesktop.org/monado/monado/merge_requests/2106))
  - math: Refactor apply_relation to handle valid/tracked flags more like OpenXR.
    ([!2186](https://gitlab.freedesktop.org/monado/monado/merge_requests/2186))
  - math: Restore upgrading of 3DOF relations with valid positions to ensure 3DOF
    devices have monado's 3DOF offset.
    ([!2239](https://gitlab.freedesktop.org/monado/monado/merge_requests/2239))
  - misc: Fix double free when shrinking typed array to zero.
    ([!2069](https://gitlab.freedesktop.org/monado/monado/merge_requests/2069))
  - ogl: Add various helper functions, and tidy code a bit.
    ([!1957](https://gitlab.freedesktop.org/monado/monado/merge_requests/1957))
  - os: Rename threading functions to more clearly state that it both stops and
    waits on the thread. Also add asserts to make sure primitives have been
    initialized.
    ([!1320](https://gitlab.freedesktop.org/monado/monado/merge_requests/1320),
    [!1324](https://gitlab.freedesktop.org/monado/monado/merge_requests/1324),
    [!1329](https://gitlab.freedesktop.org/monado/monado/merge_requests/1329),
    [!1353](https://gitlab.freedesktop.org/monado/monado/merge_requests/1353))
  - os/threading: Add mutex recursive wrapper.
    ([!1933](https://gitlab.freedesktop.org/monado/monado/merge_requests/1933))
  - os/threading: fix assert in debug build
    ([!2127](https://gitlab.freedesktop.org/monado/monado/merge_requests/2127))
  - os/time: Use timePeriod[Begin|End] when sleeping in precise sleeper
    ([!1585](https://gitlab.freedesktop.org/monado/monado/merge_requests/1585))
  - pacing: Add minimum app margin, also add `U_PACING_APP_MIN_MARGIN_MS` env var.
    ([!1961](https://gitlab.freedesktop.org/monado/monado/merge_requests/1961))
  - system_helpers: Make system devices easier to embed.
    ([!1977](https://gitlab.freedesktop.org/monado/monado/merge_requests/1977))
  - t/calibration: Add support for RGB image streams, also add a special sink
    converter helper to handle this case.
    ([!859](https://gitlab.freedesktop.org/monado/monado/merge_requests/859))
  - t/calibration: Make it possible to select number distortion parameters.
    ([!859](https://gitlab.freedesktop.org/monado/monado/merge_requests/859))
  - t/calibration: Add support for findChessboardCornersSB in calibration code.
    ([!911](https://gitlab.freedesktop.org/monado/monado/merge_requests/911))
  - t/cli: Add monado-cli slambatch command for evaluation of SLAM datasets in
    batch.
    ([!1172](https://gitlab.freedesktop.org/monado/monado/merge_requests/1172))
  - t/euroc: Add EuRoC dataset recorder for saving camera and IMU streams to disk.
    ([!1017](https://gitlab.freedesktop.org/monado/monado/merge_requests/1017))
  - t/euroc: Allow euroc recorder to start and stop recordings in the same session
    ([!1937](https://gitlab.freedesktop.org/monado/monado/merge_requests/1937))
  - t/file: Migrate calibration file format to JSON.
    ([!1005](https://gitlab.freedesktop.org/monado/monado/merge_requests/1005))
  - t/fm: Add simple FrameMat that wraps a cv::Mat, this allows us to easily pass
    cv::Mat's around without the C code needing to know about OpenCV.
    ([!825](https://gitlab.freedesktop.org/monado/monado/merge_requests/825))
  - t/hsv: Add tracing support for timing info.
    ([!858](https://gitlab.freedesktop.org/monado/monado/merge_requests/858))
  - t/psvr: Fix warnings found with GCC 13.
    ([!1921](https://gitlab.freedesktop.org/monado/monado/merge_requests/1921))
  - t/slam: Initial external SLAM tracking support, working with fork of Kimera-
    VIO.
    ([!889](https://gitlab.freedesktop.org/monado/monado/merge_requests/889))
  - t/slam: Add Basalt as a possible external SLAM system.
    ([!941](https://gitlab.freedesktop.org/monado/monado/merge_requests/941))
  - t/slam: Update SLAM interface to support dynamically query external systems for
    special features.
    ([!1016](https://gitlab.freedesktop.org/monado/monado/merge_requests/1016))
  - t/slam: Add naive prediction to the SLAM tracker.
    ([!1060](https://gitlab.freedesktop.org/monado/monado/merge_requests/1060))
  - t/slam: Add trajectory filters and use IMU for prediction in the SLAM tracker.
    ([!1067](https://gitlab.freedesktop.org/monado/monado/merge_requests/1067))
  - t/slam: Add tools for performance and accuracy evaluation of the SLAM tracker.
    ([!1152](https://gitlab.freedesktop.org/monado/monado/merge_requests/1152))
  - t/slam: Support calibration info from drivers and sending it to the external
    SLAM system.
    ([!1334](https://gitlab.freedesktop.org/monado/monado/merge_requests/1334))
  - t/slam: Add basic tracing support.
    ([!1796](https://gitlab.freedesktop.org/monado/monado/merge_requests/1796))
  - t/slam: Add reset state button
    ([!1937](https://gitlab.freedesktop.org/monado/monado/merge_requests/1937))
  - t/slam: Turn timestamp asserts into warnings
    ([!1937](https://gitlab.freedesktop.org/monado/monado/merge_requests/1937))
  - t/slam: Use locks for CSV writers
    ([!2000](https://gitlab.freedesktop.org/monado/monado/merge_requests/2000))
  - t/slam: Use newly-extracted "VIT" (visual-inertial tracking) interface project,
    version 2.0.1, to connect to SLAM trackers.
    ([!2131](https://gitlab.freedesktop.org/monado/monado/merge_requests/2131),
    [!2132](https://gitlab.freedesktop.org/monado/monado/merge_requests/2132))
  - t/slam: Send hand tracking masks to VIT system
    ([!2131](https://gitlab.freedesktop.org/monado/monado/merge_requests/2131))
  - tracking: Tidy and improve `xrt::auxiliary::tracking::FrameMat`.
    ([!2056](https://gitlab.freedesktop.org/monado/monado/merge_requests/2056))
  - u/aeg: Implement module for auto exposure and gain to help with SLAM tracking.
    ([!1291](https://gitlab.freedesktop.org/monado/monado/merge_requests/1291))
  - u/builder: Introduce new `u_builder` to make it easier to implement the
    interface function `xrt_builder::open_system`. Allowing lots of de-duplication
    of code that was exactly the same in most builders.
    ([!2057](https://gitlab.freedesktop.org/monado/monado/merge_requests/2057),
    [!2072](https://gitlab.freedesktop.org/monado/monado/merge_requests/2072))
  - u/builders: Refactor space overseer creation helper.
    ([!1987](https://gitlab.freedesktop.org/monado/monado/merge_requests/1987))
  - u/config_json: Add functionality to save/load gui state to file.
    ([!1074](https://gitlab.freedesktop.org/monado/monado/merge_requests/1074))
  - u/debug: Refactor code to be prettier and expose more conversion functions.
    ([!1874](https://gitlab.freedesktop.org/monado/monado/merge_requests/1874))
  - u/debug: Use system properties on Android for the debug settings, properties
    are prefixed with `debug.xrt.` so the property for `XRT_LOG` is
    `debug.xrt.XRT_LOG`.
    ([!1874](https://gitlab.freedesktop.org/monado/monado/merge_requests/1874))
  - u/debug_gui: Small refactor of loop and and tracing.
    ([!1814](https://gitlab.freedesktop.org/monado/monado/merge_requests/1814))
  - u/device: Added `u_device_2d_extents` and
    `u_setup_2d_extents_split_side_by_side`, this is hopefully to eliminate
    confusion: the FOV you had to give to `u_device_split_side_by_side` was a
    placeholder value, but some people thought it was the actual headset's FOV.
    ([!839](https://gitlab.freedesktop.org/monado/monado/merge_requests/839))
  - u/device: Improve comment on u_device_get_view_poses.
    ([!2023](https://gitlab.freedesktop.org/monado/monado/merge_requests/2023))
  - u/device: Add default, no-op and not implemented function helpers.
    ([!2039](https://gitlab.freedesktop.org/monado/monado/merge_requests/2039))
  - u/device: Added new function `u_device_setup_one_eye`.
    ([!2086](https://gitlab.freedesktop.org/monado/monado/merge_requests/2086))
  - u/fifo: Doc comments, and small improvements to the C++ wrapper helper.
    ([!1810](https://gitlab.freedesktop.org/monado/monado/merge_requests/1810))
  - u/file: Search more paths, and actually test if a directory is there, for
    hand-tracking models.
    ([!1831](https://gitlab.freedesktop.org/monado/monado/merge_requests/1831))
  - u/file: Changed file open mode from "r" to "rb" to ensure binary mode is used
    for reading the file. This resolves an issue where file size and read size were
    inconsistent on Windows platform.
    ([!2164](https://gitlab.freedesktop.org/monado/monado/merge_requests/2164))
  - u/frame_times_widget: Optimize FPS calculation using precomputed frame timings.
    ([!2068](https://gitlab.freedesktop.org/monado/monado/merge_requests/2068))
  - u/generic_callbacks: Fix missing include for generic callback structure.
    ([!1931](https://gitlab.freedesktop.org/monado/monado/merge_requests/1931))
  - u/json: Add cJSON C++ wrapper.
    ([!957](https://gitlab.freedesktop.org/monado/monado/merge_requests/957))
  - u/linux: Add code that raises the priority of the calling thread to realtime,
    requires the process to be run as root or have `CAP_SYS_NICE` set.
    ([!1881](https://gitlab.freedesktop.org/monado/monado/merge_requests/1881))
  - u/live_stats: Add helper to do live statistics on nano-seconds durations.
    ([!2108](https://gitlab.freedesktop.org/monado/monado/merge_requests/2108))
  - u/logging: Fix the first message always getting printed due to un-initialized
    variable.
    ([!735](https://gitlab.freedesktop.org/monado/monado/merge_requests/735))
  - u/logging: Add logging sink to intercept log messages.
    ([!1171](https://gitlab.freedesktop.org/monado/monado/merge_requests/1171))
  - u/logging: Log to stderr in Windows.
    ([!1475](https://gitlab.freedesktop.org/monado/monado/merge_requests/1475))
  - u/logging: Truncate the output of hexdump at a safer limit (16MB).
    ([!1865](https://gitlab.freedesktop.org/monado/monado/merge_requests/1865),
    [!1879](https://gitlab.freedesktop.org/monado/monado/merge_requests/1879))
  - u/logging: Refactor printing to be safer using truncating helpers, and increase
    the reuse of code.
    ([!1865](https://gitlab.freedesktop.org/monado/monado/merge_requests/1865),
    [!1865](https://gitlab.freedesktop.org/monado/monado/merge_requests/1865),
    [!1892](https://gitlab.freedesktop.org/monado/monado/merge_requests/1892))
  - u/logging: Make the CMake variable only be true on Linux.
    ([!1865](https://gitlab.freedesktop.org/monado/monado/merge_requests/1865))
  - u/logging: Add json logging, it can be enabled via the XRT_JSON_LOG env var.
    ([!1898](https://gitlab.freedesktop.org/monado/monado/merge_requests/1898))
  - u/metrics: Add code that allows writing various metrics information that can
    then be processed for a better view into the run.
    ([!1512](https://gitlab.freedesktop.org/monado/monado/merge_requests/1512),
    [!1521](https://gitlab.freedesktop.org/monado/monado/merge_requests/1521),
    [!1579](https://gitlab.freedesktop.org/monado/monado/merge_requests/1579))
  - u/native_images_debug: Add `u_native_images_debug` and `u_swapchain_debug` to
    debug `xrt_image_native` and `xrt_swapchain_native` content.
    ([!2103](https://gitlab.freedesktop.org/monado/monado/merge_requests/2103))
  - u/pacing: Add frame timing helper code designed to use Vulkan display timing
    extensions to get proper frame timing in the compositor.
    ([!697](https://gitlab.freedesktop.org/monado/monado/merge_requests/697))
  - u/pacing: Renames and improvements for frame pacing (formerly known as render
    and display timing) code and APIs.
    ([!1081](https://gitlab.freedesktop.org/monado/monado/merge_requests/1081),
    [!1104](https://gitlab.freedesktop.org/monado/monado/merge_requests/1104))
  - u/pacing: Make present_to_display_offset_ns more clear by changing the name.
    ([!1271](https://gitlab.freedesktop.org/monado/monado/merge_requests/1271))
  - u/pacing: Predict present time and then calculate display time in fake pacer.
    ([!1271](https://gitlab.freedesktop.org/monado/monado/merge_requests/1271))
  - u/pacing: Make the comp time be at least 2ms in fake pacer, this is a more
    conservative margin for when the fake pacer is used for real hardware.
    ([!1271](https://gitlab.freedesktop.org/monado/monado/merge_requests/1271))
  - u/pacing: Add vblank timing function for display control, lets the fake pacer
    properly synchronise with hardware.
    ([!1271](https://gitlab.freedesktop.org/monado/monado/merge_requests/1271))
  - u/pacing: Add variable tracking to fake pacer.
    ([!1801](https://gitlab.freedesktop.org/monado/monado/merge_requests/1801))
  - u/pacing: General improvements.
    ([!1809](https://gitlab.freedesktop.org/monado/monado/merge_requests/1809))
  - u/pacing: Add minimum compositor frame time.
    ([!1809](https://gitlab.freedesktop.org/monado/monado/merge_requests/1809))
  - u/pacing: Add minimum application frame time.
    ([!1809](https://gitlab.freedesktop.org/monado/monado/merge_requests/1809),
    [!1809](https://gitlab.freedesktop.org/monado/monado/merge_requests/1809),
    [!1828](https://gitlab.freedesktop.org/monado/monado/merge_requests/1828))
  - u/pacing: Add variable tracking integration to app pacer.
    ([!1828](https://gitlab.freedesktop.org/monado/monado/merge_requests/1828))
  - u/pacing: Add env variable to set present to display offset.
    ([!1828](https://gitlab.freedesktop.org/monado/monado/merge_requests/1828))
  - u/pacing: Add option U_PACING_APP_USE_MIN_FRAME_PERIOD to allow selecting the
    minimal frame period instead of calculated for pacing. The app is still being
    throttled, it's just different.
    ([!2076](https://gitlab.freedesktop.org/monado/monado/merge_requests/2076),
    [!2084](https://gitlab.freedesktop.org/monado/monado/merge_requests/2084))
  - u/pacing: Split submit timing into begin and end.
    ([!2108](https://gitlab.freedesktop.org/monado/monado/merge_requests/2108))
  - u/pacing: Keep track of frame times in fake pacer.
    ([!2108](https://gitlab.freedesktop.org/monado/monado/merge_requests/2108))
  - u/pacing: Do live stats tracking in fake pacer.
    ([!2108](https://gitlab.freedesktop.org/monado/monado/merge_requests/2108))
  - u/pp: Pretty print support for new `xrt_input_name` entry,
    `XRT_INPUT_GENERIC_PALM_POSE` for `XR_EXT_palm_pose`.
    ([!1896](https://gitlab.freedesktop.org/monado/monado/merge_requests/1896))
  - u/pp: Tidy and add more entries to enum printing functions.
    ([!2092](https://gitlab.freedesktop.org/monado/monado/merge_requests/2092))
  - u/pp: Add `xrt_reference_space_type` printing.
    ([!2092](https://gitlab.freedesktop.org/monado/monado/merge_requests/2092))
  - u/session: Add helper to implement `xrt_session`.
    ([!2062](https://gitlab.freedesktop.org/monado/monado/merge_requests/2062))
  - u/sink: Add tracing support to sink functions.
    ([!858](https://gitlab.freedesktop.org/monado/monado/merge_requests/858))
  - u/sink: Add a combiner sink that combines two frames into a stereo frame
    ([!934](https://gitlab.freedesktop.org/monado/monado/merge_requests/934))
  - u/space: Add local_floor to legacy helper function, making most builders
    support it automatically.
    ([!2018](https://gitlab.freedesktop.org/monado/monado/merge_requests/2018))
  - u/space: Fix build warning because of non-void function not returning
    ([!2131](https://gitlab.freedesktop.org/monado/monado/merge_requests/2131))
  - u/space_overseer: Make it possible set root as unbounded.
    ([!1621](https://gitlab.freedesktop.org/monado/monado/merge_requests/1621))
  - u/space_overseer: Add support for reference space usage.
    ([!2048](https://gitlab.freedesktop.org/monado/monado/merge_requests/2048))
  - u/space_overseer: Implement recentering for supported setups.
    ([!2055](https://gitlab.freedesktop.org/monado/monado/merge_requests/2055))
  - u/space_overseer: Use broadcast event sink for reference space changes,
    generates `xrt_session_event_reference_space_change_pending` events.
    ([!2081](https://gitlab.freedesktop.org/monado/monado/merge_requests/2081))
  - u/space_overseer: Notify the device about reference space usage.
    ([!2091](https://gitlab.freedesktop.org/monado/monado/merge_requests/2091))
  - u/system: Add helper to implement `xrt_system`.
    ([!2062](https://gitlab.freedesktop.org/monado/monado/merge_requests/2062))
  - u/system_helpers: Refactor hand-tracker helper getters.
    ([!1987](https://gitlab.freedesktop.org/monado/monado/merge_requests/1987))
  - u/system_helpers: Add static system device helper.
    ([!1992](https://gitlab.freedesktop.org/monado/monado/merge_requests/1992))
  - u/time: Add helper comparison functions.
    ([!721](https://gitlab.freedesktop.org/monado/monado/merge_requests/721))
  - u/time: Add helper to go from milliseconds to nanoseconds.
    ([!1809](https://gitlab.freedesktop.org/monado/monado/merge_requests/1809))
  - u/timing: A rather large refactor that turns makes the rendering timing helper
    be more like the frame timing helper. This also makes the rendering timing
    adjust the frame timing of the app so that latency is reduced.
    ([!721](https://gitlab.freedesktop.org/monado/monado/merge_requests/721))
  - u/trace_marker: Add trace marker support code, this code uses the Linux
    trace_marker kernel support to enable Monado to trace both function calls and
    other async events.
    ([!697](https://gitlab.freedesktop.org/monado/monado/merge_requests/697))
  - u/trace_marker: Switch from homegrown tracing code to using Percetto/Perfetto.
    ([!811](https://gitlab.freedesktop.org/monado/monado/merge_requests/811),
    [!840](https://gitlab.freedesktop.org/monado/monado/merge_requests/840))
  - u/trace_marker: Add sink categories.
    ([!858](https://gitlab.freedesktop.org/monado/monado/merge_requests/858))
  - u/truncate_printf: Add helpers that have the semantics we want for the printf
    functions [vn|sn]printf.
    ([!1865](https://gitlab.freedesktop.org/monado/monado/merge_requests/1865),
    [!1865](https://gitlab.freedesktop.org/monado/monado/merge_requests/1865),
    [!1923](https://gitlab.freedesktop.org/monado/monado/merge_requests/1923))
  - u/u_config_json: Added new parameter `uint32_t *out_view_count` to the function
    `u_config_json_get_remote_settings` to provide the ability to retrieve the view
    count from the remote settings.
    ([!2086](https://gitlab.freedesktop.org/monado/monado/merge_requests/2086))
  - u/u_distortion: Modified the function `u_distortion_cardboard_calculate` to
    accept a new parameter `struct xrt_device *xdev` for retrieving the
    `view_count` from the device. This `view_count` is then used for parameter
    settings, enhancing the functionality and flexibility of the distortion
    calculation.
    ([!2086](https://gitlab.freedesktop.org/monado/monado/merge_requests/2086))
  - u/var: Improve documentation.
    ([!1827](https://gitlab.freedesktop.org/monado/monado/merge_requests/1827))
  - u/var: Improve documentation and make `suffix_with_number` argument clearer.
    ([!1902](https://gitlab.freedesktop.org/monado/monado/merge_requests/1902))
  - u/var: Refactor code to make it easier to search for number objects.
    ([!1902](https://gitlab.freedesktop.org/monado/monado/merge_requests/1902))
  - u/var: Add `u_native_images_debug` as a tracked variable.
    ([!2103](https://gitlab.freedesktop.org/monado/monado/merge_requests/2103))
  - u/var: Protect tracker access with a mutex. Solves a race condition that may
    crash the debug gui if objects are removed using `u_var_remove_root`.
    ([!2177](https://gitlab.freedesktop.org/monado/monado/merge_requests/2177))
  - u/windows: Add helper code for various bits of Windows related things, like
    formatting error numbers into error messages. Also functions related to
    cpu priority and privilege granting of rights.
    ([!1584](https://gitlab.freedesktop.org/monado/monado/merge_requests/1584))
  - util: Add code to get a limited unique id, it's a simple 64 bit atomic counter.
    ([!1957](https://gitlab.freedesktop.org/monado/monado/merge_requests/1957))
  - vive: Add shared bindings that are used by `drv_vive` & `drv_survive`, also add
    mappings/bindings from the Touch controller to the Index Controller so games
    only providing Touch bindings works on Index controllers.
    ([!1265](https://gitlab.freedesktop.org/monado/monado/merge_requests/1265))
  - vive: Tidy the files a lot, break the calibration getters out into own file.
    ([!1792](https://gitlab.freedesktop.org/monado/monado/merge_requests/1792))
  - vive: Move the view fov calculation into the config file helper.
    ([!1792](https://gitlab.freedesktop.org/monado/monado/merge_requests/1792))
  - vive: Add hardcoded tweaks for view FoV values.
    ([!1792](https://gitlab.freedesktop.org/monado/monado/merge_requests/1792))
  - vive: Add support for Gen 3.0 and Tundra trackers.
    ([!1860](https://gitlab.freedesktop.org/monado/monado/merge_requests/1860))
  - vive: Refactor documentation and move VID and PID defines here.
    ([!1862](https://gitlab.freedesktop.org/monado/monado/merge_requests/1862))
  - vive: Add support for HTC Vive Pro 2
    ([!1911](https://gitlab.freedesktop.org/monado/monado/merge_requests/1911))
  - vive: Add C++ guards to poses header.
    ([!1929](https://gitlab.freedesktop.org/monado/monado/merge_requests/1929))
  - vive: Fix use after free, probably left over since refactor.
    ([!1960](https://gitlab.freedesktop.org/monado/monado/merge_requests/1960))
  - vive: Add builder helper to allow sharing of estimation code.
    ([!2008](https://gitlab.freedesktop.org/monado/monado/merge_requests/2008))
  - vk: Add more functions to `vk_bundle` struct.
    ([!721](https://gitlab.freedesktop.org/monado/monado/merge_requests/721),
    [!721](https://gitlab.freedesktop.org/monado/monado/merge_requests/721),
    [!841](https://gitlab.freedesktop.org/monado/monado/merge_requests/841),
    [!1142](https://gitlab.freedesktop.org/monado/monado/merge_requests/1142),
    [!1820](https://gitlab.freedesktop.org/monado/monado/merge_requests/1820))
  - vk: Make it possible to create a compute only queue.
    ([!841](https://gitlab.freedesktop.org/monado/monado/merge_requests/841))
  - vk: Refactor and tidy extension handling.
    ([!841](https://gitlab.freedesktop.org/monado/monado/merge_requests/841))
  - vk: Add support for `VK_EXT_robustness2`
    ([!841](https://gitlab.freedesktop.org/monado/monado/merge_requests/841))
  - vk: Add code to handle optional device features.
    ([!841](https://gitlab.freedesktop.org/monado/monado/merge_requests/841))
  - vk: Add helpers to manage command buffers and create various state objects.
    ([!982](https://gitlab.freedesktop.org/monado/monado/merge_requests/982))
  - vk: Refactor and rename various function related to compositor swapchain
    images and their flags. These changes makes it clear it's only used for these
    images and image views.
    ([!1128](https://gitlab.freedesktop.org/monado/monado/merge_requests/1128))
  - vk: Check which fence types can be imported and exported on the device.
    ([!1142](https://gitlab.freedesktop.org/monado/monado/merge_requests/1142))
  - vk: Add `XRT_CHECK_RESULT` to sync functions.
    ([!1166](https://gitlab.freedesktop.org/monado/monado/merge_requests/1166))
  - vk: Refactor bundle functions into a file of their own.
    ([!1203](https://gitlab.freedesktop.org/monado/monado/merge_requests/1203))
  - vk: Separate printing functions into their own file.
    ([!1203](https://gitlab.freedesktop.org/monado/monado/merge_requests/1203),
    [!1203](https://gitlab.freedesktop.org/monado/monado/merge_requests/1203),
    [!1942](https://gitlab.freedesktop.org/monado/monado/merge_requests/1942))
  - vk: Print out information about the opened device.
    ([!1270](https://gitlab.freedesktop.org/monado/monado/merge_requests/1270))
  - vk: Make `VK_KHR_external_[fence|semaphore]_fd` optional. This is helpful
    for CI where only lavapipe can be used which does not support those
    extensions.
    ([!1270](https://gitlab.freedesktop.org/monado/monado/merge_requests/1270))
  - vk: Relax the compute-only queue search to fall back to any queue that supports
    compute.
    ([!1404](https://gitlab.freedesktop.org/monado/monado/merge_requests/1404))
  - vk: Add new command buffer helpers and `vk_cmd_pool` helper class.
    ([!1748](https://gitlab.freedesktop.org/monado/monado/merge_requests/1748))
  - vk: Remove the global command buffer pool.
    ([!1748](https://gitlab.freedesktop.org/monado/monado/merge_requests/1748))
  - vk: Refactor `vk_csci_get_image_usage_flags` to not always set the sampled bit
    and other bits that where always set. Also tidy with a nice define for
    checking.
    ([!1763](https://gitlab.freedesktop.org/monado/monado/merge_requests/1763))
  - vk: Add new cmd buffer helper file `vk_cmd.[h|c]`, these does not use the
    global command pool.
    ([!1766](https://gitlab.freedesktop.org/monado/monado/merge_requests/1766))
  - vk: Add copy and blit command buffer writer helpers to `vk_cmd.[h|c]`.
    ([!1766](https://gitlab.freedesktop.org/monado/monado/merge_requests/1766))
  - vk: Add `vk_surface_info` helper for `VkSurfaceKHR` information gathering.
    ([!1801](https://gitlab.freedesktop.org/monado/monado/merge_requests/1801))
  - vk: Expand readback pool to be able to set Vulkan format.
    ([!1820](https://gitlab.freedesktop.org/monado/monado/merge_requests/1820))
  - vk: Add helper function to name Vulkan objects using `VK_EXT_debug_marker`,
    useful when debugging validation errors.
    ([!1877](https://gitlab.freedesktop.org/monado/monado/merge_requests/1877))
  - vk: Name all fence objects with helpers.
    ([!1877](https://gitlab.freedesktop.org/monado/monado/merge_requests/1877))
  - vk: Add two call helper for getting instance extensions, and use it.
    ([!1885](https://gitlab.freedesktop.org/monado/monado/merge_requests/1885))
  - vk: Add function to check required instance extensions.
    ([!1885](https://gitlab.freedesktop.org/monado/monado/merge_requests/1885),
    [!1885](https://gitlab.freedesktop.org/monado/monado/merge_requests/1885),
    [!1973](https://gitlab.freedesktop.org/monado/monado/merge_requests/1973))
  - vk: Add and use enumeration helpers.
    ([!1894](https://gitlab.freedesktop.org/monado/monado/merge_requests/1894),
    [!1940](https://gitlab.freedesktop.org/monado/monado/merge_requests/1940))
  - vk: Add string return function for VkSharingMode.
    ([!1940](https://gitlab.freedesktop.org/monado/monado/merge_requests/1940))
  - vk: Add string return functions for bitfield values, also improving the old
    bitfield string functions. The new way lets the caller deal with unknown bits.
    ([!1940](https://gitlab.freedesktop.org/monado/monado/merge_requests/1940))
  - vk: Add printers for `VkSurface` and `VkSwapchain` create info structs.
    ([!1940](https://gitlab.freedesktop.org/monado/monado/merge_requests/1940))
  - vk: When listing GPUs, also output device type.
    ([!1942](https://gitlab.freedesktop.org/monado/monado/merge_requests/1942))
  - vk: Make sure to print the first GPU as well when selecting physical device.
    ([!1942](https://gitlab.freedesktop.org/monado/monado/merge_requests/1942))
  - vk: Init vk bundle with `shaderImageGatherExtended` enabled if supported.
    ([!1959](https://gitlab.freedesktop.org/monado/monado/merge_requests/1959))
  - vk: Use `VK_CHK_WITH_RET` instead of `vk_check_error`.
    ([!1971](https://gitlab.freedesktop.org/monado/monado/merge_requests/1971))
  - vk: Rename and add more variants of return checking defines, making the define
    now be all caps so it's easier to see if it effects flow control.
    ([!1971](https://gitlab.freedesktop.org/monado/monado/merge_requests/1971),
    [!1971](https://gitlab.freedesktop.org/monado/monado/merge_requests/1971),
    [!1417](https://gitlab.freedesktop.org/monado/monado/merge_requests/1417))
  - vk: Add `vk_print_result` helper, used in return checking defines but can also
    be used outside of them.
    ([!1971](https://gitlab.freedesktop.org/monado/monado/merge_requests/1971),
    [!1971](https://gitlab.freedesktop.org/monado/monado/merge_requests/1971),
    [!2050](https://gitlab.freedesktop.org/monado/monado/merge_requests/2050))
  - vk: Add two mini define helpers in their own header (`D` and `DF`) which was
    redefined in multiple places in the source code. Keep in own header to not
    clutter namespace.
    ([!1971](https://gitlab.freedesktop.org/monado/monado/merge_requests/1971))
  - vk: Add debug inserting helper function and use it for inserting renderdoc
    frame delimiter in Vulkan client
    ([!2005](https://gitlab.freedesktop.org/monado/monado/merge_requests/2005))
  - vk: Change the naming function to use the extension `VK_EXT_debug_utils`, which
    has been included in core with 1.3, instead of the old `VK_EXT_debug_marker`
    extension. Also make the naming function type safe.
    ([!2006](https://gitlab.freedesktop.org/monado/monado/merge_requests/2006),
    [!2014](https://gitlab.freedesktop.org/monado/monado/merge_requests/2014))
  - vk: Fix swapchain leak on Android due to it having different Vulkan import
    behavior.
    ([!2042](https://gitlab.freedesktop.org/monado/monado/merge_requests/2042))
  - vk: Use formats list from `xrt_swapchain_create_info` in `create_image`.
    ([!2049](https://gitlab.freedesktop.org/monado/monado/merge_requests/2049),
    [!2100](https://gitlab.freedesktop.org/monado/monado/merge_requests/2100))
  - vk: Pass create mutable format bit if usage flag is set.
    ([!2100](https://gitlab.freedesktop.org/monado/monado/merge_requests/2100))
  - vk: Return `VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT` for
    `XRT_SWAPCHAIN_USAGE_UNORDERED_ACCESS` from `vk_csci_get_image_usage_flags`.
    ([!2103](https://gitlab.freedesktop.org/monado/monado/merge_requests/2103))
  - vk: Add enumerators for two surface properties.
    ([!2104](https://gitlab.freedesktop.org/monado/monado/merge_requests/2104))
  - vk: Add `vk_enumerate_swapchain_images`.
    ([!2104](https://gitlab.freedesktop.org/monado/monado/merge_requests/2104))
  - vk: Tidy surface info function.
    ([!2104](https://gitlab.freedesktop.org/monado/monado/merge_requests/2104))
  - vk: Extend command buffer wait timeout to ~10 seconds. This is necessary
    because in some platforms (such as Windows 10, NVIDIA RTX 3080Ti) the OpenXR
    CTS will trigger an issue when the GPU memory fills, where the system hangs
    for over one second during a paging queue operation.
    ([!2205](https://gitlab.freedesktop.org/monado/monado/merge_requests/2205))
- Documentation
  - Add documentation for how to write changelogs in the conventions page.
    ([!1872](https://gitlab.freedesktop.org/monado/monado/merge_requests/1872),
    [!2120](https://gitlab.freedesktop.org/monado/monado/merge_requests/2120))
  - Add documentation category in changelog documentation.
    ([!1878](https://gitlab.freedesktop.org/monado/monado/merge_requests/1878))
  - Add doxygen-awesome theme
    ([!1883](https://gitlab.freedesktop.org/monado/monado/merge_requests/1883),
    [!1888](https://gitlab.freedesktop.org/monado/monado/merge_requests/1888))
  - Don't build documentation by default, it is fairly heavy for end users. Also
    makes the CI scripts cleaner as they don't need to disabled it everywhere.
    ([!1891](https://gitlab.freedesktop.org/monado/monado/merge_requests/1891))
  - README: Add Debian/Ubuntu package for libudev.
    ([!1918](https://gitlab.freedesktop.org/monado/monado/merge_requests/1918))
  - README: Clarify Vulkan SDK requirement on Windows.
    ([!1919](https://gitlab.freedesktop.org/monado/monado/merge_requests/1919))
  - README: Add some Debian/Ubuntu packages.
    ([!1923](https://gitlab.freedesktop.org/monado/monado/merge_requests/1923))
  - comments: Lots of smaller documentation comment fixes.
    ([!1953](https://gitlab.freedesktop.org/monado/monado/merge_requests/1953),
    [!2037](https://gitlab.freedesktop.org/monado/monado/merge_requests/2037),
    [!2070](https://gitlab.freedesktop.org/monado/monado/merge_requests/2070),
    [!2085](https://gitlab.freedesktop.org/monado/monado/merge_requests/2085))
  - Add page with information for Linux packagers.
    ([!2195](https://gitlab.freedesktop.org/monado/monado/merge_requests/2195))
  - doc: Add Ubuntu 24.04 as supported OS [NFC]
    ([!2202](https://gitlab.freedesktop.org/monado/monado/merge_requests/2202))
- Misc. Features
  - Add JSON Schema for config files.
    ([!785](https://gitlab.freedesktop.org/monado/monado/merge_requests/785),
    [#82](https://gitlab.freedesktop.org/monado/monado/issues/82))
  - Add `cmake-format` config files and `scripts/format-cmake.sh` to keep our build
    system tidy.
    ([!984](https://gitlab.freedesktop.org/monado/monado/merge_requests/984),
    [#72](https://gitlab.freedesktop.org/monado/monado/issues/72),
    [!1342](https://gitlab.freedesktop.org/monado/monado/merge_requests/1342))
  - Add Nix flake files so that people that use the nix package manager can have an
    instant Monado development environment.
    ([!2152](https://gitlab.freedesktop.org/monado/monado/merge_requests/2152))
  - Android: Update activity and service icons to the new official Monado logos,
    and use a modified version for in-process builds to indicate they are mainly
    for debugging.
    ([!2208](https://gitlab.freedesktop.org/monado/monado/merge_requests/2208))
  - For code that is implemented in C++, note that the default standard mode is now
    C++17 across all platforms and modules, instead of a mix of 14 and 17 like
    before. The CI remains the decider of what functionality is available, as it
    contains the oldest distribution we support (Debian Buster).
    ([!809](https://gitlab.freedesktop.org/monado/monado/merge_requests/809))
  - Implement tracking overrides using wrapper devices and add a tracking override
    configuration gui.
    ([!695](https://gitlab.freedesktop.org/monado/monado/merge_requests/695))
  - In `flake.nix` and `flake.lock`, updates `nixpkgs` to the version where the
    missing dependency was added.
    ([!2184](https://gitlab.freedesktop.org/monado/monado/merge_requests/2184))
  - Introduce `.mailmap` file.
    ([!2041](https://gitlab.freedesktop.org/monado/monado/merge_requests/2041))
  - Introduce visual-inertial tracking interface header and remove the old SLAM
    tracker interface, remove XRT_HAVE_BASALT and XRT_HAVE_KIMERA from CMake
    ([!2058](https://gitlab.freedesktop.org/monado/monado/merge_requests/2058))
  - More improvements to the Android port.
    ([!676](https://gitlab.freedesktop.org/monado/monado/merge_requests/676),
    [!703](https://gitlab.freedesktop.org/monado/monado/merge_requests/703),
    [!783](https://gitlab.freedesktop.org/monado/monado/merge_requests/783),
    [!808](https://gitlab.freedesktop.org/monado/monado/merge_requests/808),
    [!817](https://gitlab.freedesktop.org/monado/monado/merge_requests/817),
    [!820](https://gitlab.freedesktop.org/monado/monado/merge_requests/820),
    [!918](https://gitlab.freedesktop.org/monado/monado/merge_requests/918),
    [!920](https://gitlab.freedesktop.org/monado/monado/merge_requests/920),
    [!942](https://gitlab.freedesktop.org/monado/monado/merge_requests/942),
    [!1020](https://gitlab.freedesktop.org/monado/monado/merge_requests/1020),
    [!1178](https://gitlab.freedesktop.org/monado/monado/merge_requests/1178),
    [!1341](https://gitlab.freedesktop.org/monado/monado/merge_requests/1341),
    [!1357](https://gitlab.freedesktop.org/monado/monado/merge_requests/1357),
    [!1369](https://gitlab.freedesktop.org/monado/monado/merge_requests/1369),
    [!1372](https://gitlab.freedesktop.org/monado/monado/merge_requests/1372),
    [!1377](https://gitlab.freedesktop.org/monado/monado/merge_requests/1377),
    [!1385](https://gitlab.freedesktop.org/monado/monado/merge_requests/1385),
    [!2232](https://gitlab.freedesktop.org/monado/monado/merge_requests/2232),
    [!2204](https://gitlab.freedesktop.org/monado/monado/merge_requests/2204))
  - More work on the Windows port: fix timing, waiting, sleeping, handling the
    message queue.
    ([!739](https://gitlab.freedesktop.org/monado/monado/merge_requests/739),
    [!743](https://gitlab.freedesktop.org/monado/monado/merge_requests/743),
    [!1322](https://gitlab.freedesktop.org/monado/monado/merge_requests/1322))
  - Sign main branch CI-generated APKs for Android release builds.
    ([!2209](https://gitlab.freedesktop.org/monado/monado/merge_requests/2209))
  - a/gst: Add a small and fairly naive framework for integrating gstreamer
    pipelines into Monado pipelines. Enough to be able to push frames into it
    and use various encoder elements.
    ([!715](https://gitlab.freedesktop.org/monado/monado/merge_requests/715),
    [!1966](https://gitlab.freedesktop.org/monado/monado/merge_requests/1966))
  - cmake: remove unused ffmpeg dependency
    ([!2122](https://gitlab.freedesktop.org/monado/monado/merge_requests/2122))
  - cmake: enable policy CMP0083 for position-independent-executable support
    ([!2168](https://gitlab.freedesktop.org/monado/monado/merge_requests/2168),
    [#337](https://gitlab.freedesktop.org/monado/monado/issues/337))
  - cmake: add VERSION and SOVERSION properties to monado shared library
    ([!2170](https://gitlab.freedesktop.org/monado/monado/merge_requests/2170))
  - ext/imgui: Add helper to draw a image/texture with a cleared background color.
    ([!1957](https://gitlab.freedesktop.org/monado/monado/merge_requests/1957))
  - ext/openxr: Update headers to 1.0.28.
    ([!1900](https://gitlab.freedesktop.org/monado/monado/merge_requests/1900))
  - ext/openxr: Bump OpenXR headers to 1.0.32
    ([!2063](https://gitlab.freedesktop.org/monado/monado/merge_requests/2063),
    [!2069](https://gitlab.freedesktop.org/monado/monado/merge_requests/2069))
  - ext/openxr: Bump OpenXR headers to 1.0.33
    ([!2111](https://gitlab.freedesktop.org/monado/monado/merge_requests/2111))
  - ext/openxr: Bump OpenXR headers to 1.0.34
    ([!2148](https://gitlab.freedesktop.org/monado/monado/merge_requests/2148))
  - external: Update android-jni-wrap, add additional wrapped methods.
    ([!1939](https://gitlab.freedesktop.org/monado/monado/merge_requests/1939),
    [!1963](https://gitlab.freedesktop.org/monado/monado/merge_requests/1963),
    [!2176](https://gitlab.freedesktop.org/monado/monado/merge_requests/2176))
  - external/glad: Add EGL extension EGL_KHR_no_config_context.
    ([!705](https://gitlab.freedesktop.org/monado/monado/merge_requests/705))
  - external/jni: Add argument to specify display mode id for surface creation.
    ([!2010](https://gitlab.freedesktop.org/monado/monado/merge_requests/2010))
  - gui: Use a single imgui.ini file from the config directory
    ([!1290](https://gitlab.freedesktop.org/monado/monado/merge_requests/1290))
  - imgui: Add ImPlot demo window.
    ([!692](https://gitlab.freedesktop.org/monado/monado/merge_requests/692))
  - m/vec2: Add float array helper
    ([!1928](https://gitlab.freedesktop.org/monado/monado/merge_requests/1928))
  - m/vec3: Add float array helper
    ([!1928](https://gitlab.freedesktop.org/monado/monado/merge_requests/1928))
  - scripts: Add Include-What-You-Use (IWYU) helper scripts.
    ([!1229](https://gitlab.freedesktop.org/monado/monado/merge_requests/1229))
  - t/cli: Add support for new dynamic device roles.
    ([!1992](https://gitlab.freedesktop.org/monado/monado/merge_requests/1992))
  - t/cli: Add support for `xrt_system`.
    ([!2062](https://gitlab.freedesktop.org/monado/monado/merge_requests/2062))
  - t/cli: Add new `info` command that prints information about the system, this is
    for end-user reports of failures to start `monado-service`.
    ([!2094](https://gitlab.freedesktop.org/monado/monado/merge_requests/2094))
  - t/common: Refactor the builders so it will be easier to add hotswap support.
    ([!1987](https://gitlab.freedesktop.org/monado/monado/merge_requests/1987))
  - t/common: Add support for new dynamic device roles.
    ([!1992](https://gitlab.freedesktop.org/monado/monado/merge_requests/1992),
    [!1992](https://gitlab.freedesktop.org/monado/monado/merge_requests/1992),
    [!1999](https://gitlab.freedesktop.org/monado/monado/merge_requests/1999),
    [#296](https://gitlab.freedesktop.org/monado/monado/issues/296),
    [!2020](https://gitlab.freedesktop.org/monado/monado/merge_requests/2020))
  - t/common: Refactor lighthouse builder to use vive_builder helper.
    ([!2008](https://gitlab.freedesktop.org/monado/monado/merge_requests/2008))
  - t/common: Add support for `xrt_system` and `xrt_session`.
    ([!2062](https://gitlab.freedesktop.org/monado/monado/merge_requests/2062))
  - t/common: Implement SteamVR builder.
    ([!2077](https://gitlab.freedesktop.org/monado/monado/merge_requests/2077))
  - t/ctl: Use common client connection connect code.
    ([!1875](https://gitlab.freedesktop.org/monado/monado/merge_requests/1875))
  - t/ctl: Support recentering of local spaces.
    ([!2055](https://gitlab.freedesktop.org/monado/monado/merge_requests/2055))
  - t/libmonado: Add support for dynamic device roles.
    ([!2013](https://gitlab.freedesktop.org/monado/monado/merge_requests/2013))
  - t/libmonado: Support recentering of local spaces.
    ([!2055](https://gitlab.freedesktop.org/monado/monado/merge_requests/2055))
  - t/libmonado: Support getting serial number from the device.
    ([!2099](https://gitlab.freedesktop.org/monado/monado/merge_requests/2099))
  - t/sdl_test: Use new OpenGL helpers to import swapchain images.
    ([!1957](https://gitlab.freedesktop.org/monado/monado/merge_requests/1957))
  - t/sdl_test: Add support for new dynamic device roles.
    ([!1992](https://gitlab.freedesktop.org/monado/monado/merge_requests/1992))
  - t/sdl_test: Add support for `xrt_system` and `xrt_session`, also remove old
    events code.
    ([!2062](https://gitlab.freedesktop.org/monado/monado/merge_requests/2062))
  - t/service-lib: Increase the duration of the starting timeout for the IPC.
    ([!2015](https://gitlab.freedesktop.org/monado/monado/merge_requests/2015))
- Misc. Fixes
  - Allow OpenGL to be found on \*nix without requiring GLX, which should allow
    a Wayland-only build.
    ([!963](https://gitlab.freedesktop.org/monado/monado/merge_requests/963),
    [#132](https://gitlab.freedesktop.org/monado/monado/issues/132))
  - Ensure we are always initializing our mutexes.
    ([!737](https://gitlab.freedesktop.org/monado/monado/merge_requests/737))
  - Fix build issue with Wayland on some distributions.
    ([!1396](https://gitlab.freedesktop.org/monado/monado/merge_requests/1396),
    [#175](https://gitlab.freedesktop.org/monado/monado/issues/175))
  - Fix several minor bindings and input profile issues.
    ([!2190](https://gitlab.freedesktop.org/monado/monado/merge_requests/2190))
  - Make config file reading more robust.
    ([!785](https://gitlab.freedesktop.org/monado/monado/merge_requests/785))
  - Move C++-only functionality into the newly-conventional namespaces.
    ([!810](https://gitlab.freedesktop.org/monado/monado/merge_requests/810))
  - Update vendored Catch2 to 2.13.10 to fix build issue.
    ([!1561](https://gitlab.freedesktop.org/monado/monado/merge_requests/1561),
    [#221](https://gitlab.freedesktop.org/monado/monado/issues/221))
  - Update outdated URLs, email addresses, and names.
    ([!2041](https://gitlab.freedesktop.org/monado/monado/merge_requests/2041))
  - Update gitignore to exclude files intentionally in the repo.
    ([!2137](https://gitlab.freedesktop.org/monado/monado/merge_requests/2137),
    [#261](https://gitlab.freedesktop.org/monado/monado/issues/261))
  - Various small warning fixes all over the codebase.
    ([!1869](https://gitlab.freedesktop.org/monado/monado/merge_requests/1869),
    [!2126](https://gitlab.freedesktop.org/monado/monado/merge_requests/2126))
  - Various spelling fixes all over the codebase.
    ([!1871](https://gitlab.freedesktop.org/monado/monado/merge_requests/1871))
  - build: Removed incorrect hidapi dependency from Rift S driver
    ([!2227](https://gitlab.freedesktop.org/monado/monado/merge_requests/2227))
  - ci: Miscellaneous fixes, improvements, and updates.
    ([!1886](https://gitlab.freedesktop.org/monado/monado/merge_requests/1886),
    [!2029](https://gitlab.freedesktop.org/monado/monado/merge_requests/2029),
    [!2031](https://gitlab.freedesktop.org/monado/monado/merge_requests/2031),
    [!2122](https://gitlab.freedesktop.org/monado/monado/merge_requests/2122),
    [!2181](https://gitlab.freedesktop.org/monado/monado/merge_requests/2181),
    [!2196](https://gitlab.freedesktop.org/monado/monado/merge_requests/2196),
    [!2197](https://gitlab.freedesktop.org/monado/monado/merge_requests/2197))
  - ci: use proclamation 2.0.0
    ([!2123](https://gitlab.freedesktop.org/monado/monado/merge_requests/2123))
  - ci: Add CI for Ubuntu 24.04
    ([!2202](https://gitlab.freedesktop.org/monado/monado/merge_requests/2202))
  - cmake: Build system will now error out, rather than silently disable the
    option,
    if you specifically enable an option whose dependencies are unavailable.
    ([!1262](https://gitlab.freedesktop.org/monado/monado/merge_requests/1262))
  - cmake: Build system option `XRT_HAVE_SLAM` has been renamed to
    `XRT_FEATURE_SLAM` to more accurately describe it, with corresponding move
    from `xrt_config_have.h` to `xrt_config_build.h`.
    ([!1262](https://gitlab.freedesktop.org/monado/monado/merge_requests/1262))
  - cmake: Handle multiple include of compiler flags.
    ([!1882](https://gitlab.freedesktop.org/monado/monado/merge_requests/1882))
  - cmake: Fix GetGitRevisionDescription cmake module for MSys
    ([!1944](https://gitlab.freedesktop.org/monado/monado/merge_requests/1944))
  - cmake: Fix build with SDL2 on Alpine Linux.
    ([!2031](https://gitlab.freedesktop.org/monado/monado/merge_requests/2031))
  - cmake: Suppress warnings from external headers.
    ([!2037](https://gitlab.freedesktop.org/monado/monado/merge_requests/2037))
  - cmake: Update CMake modules from upstream repositories.
    ([!2040](https://gitlab.freedesktop.org/monado/monado/merge_requests/2040),
    [!2041](https://gitlab.freedesktop.org/monado/monado/merge_requests/2041),
    [!2045](https://gitlab.freedesktop.org/monado/monado/merge_requests/2045))
  - cmake: add wayland-client include directory to comp_main target
    ([!2141](https://gitlab.freedesktop.org/monado/monado/merge_requests/2141))
  - d/twrap: Correct axis assignments for poses provided by basalt VIO/SLAM, so
    they match the OpenXR axis definition.
    ([!2228](https://gitlab.freedesktop.org/monado/monado/merge_requests/2228))
  - ext/oxr: Add missing headers for unpublished monado extensions:
    `XR_MNDX_ball_on_a_stick_controller` and `XR_MNDX_hydra`.
    ([!1890](https://gitlab.freedesktop.org/monado/monado/merge_requests/1890))
  - gitignore: Ignore pyenv local python version file
    ([!2002](https://gitlab.freedesktop.org/monado/monado/merge_requests/2002))
  - gradle: Migrate deprecated gradle's flavorDimension and buildToolsVersion
    ([!2002](https://gitlab.freedesktop.org/monado/monado/merge_requests/2002))
  - h/mercury: Fix warnings found with GCC 13.
    ([!1921](https://gitlab.freedesktop.org/monado/monado/merge_requests/1921))
  - jnipp: Update/patch to fix issues, including crashes.
    ([!2200](https://gitlab.freedesktop.org/monado/monado/merge_requests/2200),
    [!2226](https://gitlab.freedesktop.org/monado/monado/merge_requests/2226))
  - misc: Various NFC format fixes and a removal of unused define.
    ([!1946](https://gitlab.freedesktop.org/monado/monado/merge_requests/1946))
  - scripts: Add regex based ignores for codespell, needed because the ignore words
    list isn't properly case sensitive.
    ([!1861](https://gitlab.freedesktop.org/monado/monado/merge_requests/1861))
  - t/android_common: Specify mutability flag for PendingIntent object, required
    for Android S+ (version 31 and above).
    ([!1948](https://gitlab.freedesktop.org/monado/monado/merge_requests/1948))
  - t/common: Make it possible to build the RGB builder without PSVR driver.
    ([!1918](https://gitlab.freedesktop.org/monado/monado/merge_requests/1918))
  - t/common: Fix warnings found with GCC 13.
    ([!1921](https://gitlab.freedesktop.org/monado/monado/merge_requests/1921))
  - t/common: Fix building the Lighthouse builder without the Vive driver.
    ([!1922](https://gitlab.freedesktop.org/monado/monado/merge_requests/1922))
  - t/common: Fix creation on no driver available, only say we
    can create a system if we have a driver in legacy builder.
    ([!1996](https://gitlab.freedesktop.org/monado/monado/merge_requests/1996))
  - t/common: Use new `u_builder` helper in most builder.
    ([!2057](https://gitlab.freedesktop.org/monado/monado/merge_requests/2057))
  - t/ctl: Use correct ipc call for toggling client I/O.
    ([!1909](https://gitlab.freedesktop.org/monado/monado/merge_requests/1909))
  - t/north_star: In the North Star builder, add the SLAM device after
    the HMD device to avoid monado-service crash due to misordering.
    ([!2228](https://gitlab.freedesktop.org/monado/monado/merge_requests/2228))
  - t/sdl_test: Compile as UTF-8 to fix MSVC warning.
    ([!1816](https://gitlab.freedesktop.org/monado/monado/merge_requests/1816))
  - t/sdl_test: sdl-test needs OpenGL4.5
    ([!1945](https://gitlab.freedesktop.org/monado/monado/merge_requests/1945))
  - vcpkg: Remove SDL "base" feature. It has been removed upstream, see
    [MR](https://github.com/microsoft/vcpkg/commit/ea9f45d1bc03efbf43a3bbd0788d6a43
    3b8fe445).
    Monado builds on Windows and the debug gui works (`XRT_DEBUG_GUI=1`).
    ([!2065](https://gitlab.freedesktop.org/monado/monado/merge_requests/2065))

## Monado 21.0.0 (2021-01-28)

- Major changes
  - Adds a initial SteamVR driver state tracker and target that produces a SteamVR
    plugin that enables any Monado hardware driver to be used in SteamVR. This is
    the initial upstreaming of this code and has some limitations, like only having
    working input when emulating a Index controller.
    ([!583](https://gitlab.freedesktop.org/monado/monado/merge_requests/583))
- XRT Interface
  - Add `xrt_binding_profile` struct, related pair structs and fields on
    `xrt_device` to allow to move the static rebinding of inputs and outputs into
    device drivers. This makes it easier to get a overview in the driver itself
    which bindings it can bind to.
    ([!587](https://gitlab.freedesktop.org/monado/monado/merge_requests/587))
  - xrt: Generate bindings for Monado and SteamVR from json.
    ([!638](https://gitlab.freedesktop.org/monado/monado/merge_requests/638))
  - xrt: Introduce `xrt_system_compositor`, it is basically a analogous to
    `XrSystemID` but instead of being a fully fledged xrt_system this is only the
    compositor part of it. Also fold the `prepare_session` function into the create
    native compositor function to simplify the interface.
    ([!652](https://gitlab.freedesktop.org/monado/monado/merge_requests/652))
  - Expose more information on the frameservers, like product, manufacturer and
    serial.
    ([!665](https://gitlab.freedesktop.org/monado/monado/merge_requests/665))
  - Add `XRT_FORMAT_BAYER_GR8` format.
    ([!665](https://gitlab.freedesktop.org/monado/monado/merge_requests/665))
- State Trackers
  - st/oxr: Add OXR_FRAME_TIMING_SPEW for basic frame timing debug output.
    ([!591](https://gitlab.freedesktop.org/monado/monado/merge_requests/591))
  - OpenXR: Make sure to restore old EGL display/context/drawables when creating a
    client EGL compositor.
    ([!602](https://gitlab.freedesktop.org/monado/monado/merge_requests/602))
  - GUI: Expand with support for controlling the remote driver hand tracking.
    ([!604](https://gitlab.freedesktop.org/monado/monado/merge_requests/604))
  - st/oxr: Implement XR_KHR_vulkan_enable2
    ([!633](https://gitlab.freedesktop.org/monado/monado/merge_requests/633))
  - st/oxr: Add OXR_TRACKING_ORIGIN_OFFSET_{X,Y,Z} env variables as a quick way to
    tweak 6dof tracking origins.
    ([!634](https://gitlab.freedesktop.org/monado/monado/merge_requests/634))
  - OpenXR: Be more relaxed with Quat validation, spec says within 1% of unit
    length, normalize if not within float epsilon.
    ([!659](https://gitlab.freedesktop.org/monado/monado/merge_requests/659))
- Drivers
  - ns: Fix memory leak in math code.
    ([!564](https://gitlab.freedesktop.org/monado/monado/merge_requests/564))
  - psvr: Rename some variables for better readability.
    ([!597](https://gitlab.freedesktop.org/monado/monado/merge_requests/597))
  - openhmd: Fix viewport calculation of rotated displays.
    ([!600](https://gitlab.freedesktop.org/monado/monado/merge_requests/600))
  - remote: Add support for simulated hand tracking, this is based on the curl
    model
    that is used by the Valve Index Controller.
    ([!604](https://gitlab.freedesktop.org/monado/monado/merge_requests/604))
  - android: Acquire device display metrics from system.
    ([!611](https://gitlab.freedesktop.org/monado/monado/merge_requests/611))
  - openhmd: Rotate DK2 display correctly.
    ([!628](https://gitlab.freedesktop.org/monado/monado/merge_requests/628))
  - d/psmv: The motor on zcmv1 does not rumble at amplitudes < 0.25. Linear rescale
    amplitude into [0.25, 1] range.
    ([!636](https://gitlab.freedesktop.org/monado/monado/merge_requests/636))
  - v4l2: Expose more information through new fields in XRT interface.
    ([!665](https://gitlab.freedesktop.org/monado/monado/merge_requests/665))
  - v4l2: Allocate more buffers when streaming data.
    ([!665](https://gitlab.freedesktop.org/monado/monado/merge_requests/665))
- IPC
  - ipc: Port IPC to u_logging.
    ([!601](https://gitlab.freedesktop.org/monado/monado/merge_requests/601))
  - ipc: Make OXR_DEBUG_GUI work with monado-service.
    ([!622](https://gitlab.freedesktop.org/monado/monado/merge_requests/622))
- Compositor
  - comp: Add basic frame timing information to XRT_COMPOSITOR_LOG=trace.
    ([!591](https://gitlab.freedesktop.org/monado/monado/merge_requests/591))
  - main: Refactor how the compositor interacts with targets, the goal is to enable
    the compositor to render to destinations that isn't backed by a `VkSwapchain`.
    Introduce `comp_target` and remove `comp_window`, also refactor `vk_swapchain`
    to be a sub-class of `comp_target` named `comp_target_swapchain`, the window
    backends now sub class `comp_target_swapchain`.
    ([!599](https://gitlab.freedesktop.org/monado/monado/merge_requests/599))
  - Implement support for XR_KHR_composition_layer_equirect (equirect1).
    ([!620](https://gitlab.freedesktop.org/monado/monado/merge_requests/620),
    [!624](https://gitlab.freedesktop.org/monado/monado/merge_requests/624))
  - comp: Improve thread safety. Resolve issues in multithreading CTS.
    ([!645](https://gitlab.freedesktop.org/monado/monado/merge_requests/645))
  - main: Lower priority on sRGB format. This works around a bug in the OpenXR CTS
    and mirrors better what at least on other OpenXR runtime does.
    ([!671](https://gitlab.freedesktop.org/monado/monado/merge_requests/671))
- Helper Libraries
  - os/time: Make timespec argument const.
    ([!597](https://gitlab.freedesktop.org/monado/monado/merge_requests/597))
  - os/time: Add a Linux specific way to get the realtime clock (for RealSense).
    ([!597](https://gitlab.freedesktop.org/monado/monado/merge_requests/597))
  - math: Make sure that we do not drop and positions in poses when the other pose
    has a non-valid position.
    ([!603](https://gitlab.freedesktop.org/monado/monado/merge_requests/603))
  - aux/vk: `vk_create_device` now takes in a list of Vulkan device extensions.
    ([!605](https://gitlab.freedesktop.org/monado/monado/merge_requests/605))
  - Port everything to u_logging.
    ([!627](https://gitlab.freedesktop.org/monado/monado/merge_requests/627))
  - u/hand_tracking: Tweak finger curl model making it easier to grip ingame
    objects.
    ([!635](https://gitlab.freedesktop.org/monado/monado/merge_requests/635))
  - math: Add math_quat_validate_within_1_percent function.
    ([!659](https://gitlab.freedesktop.org/monado/monado/merge_requests/659))
  - u/sink: Add Bayer format converter.
    ([!665](https://gitlab.freedesktop.org/monado/monado/merge_requests/665))
  - u/distortion: Improve both Vive and Index distortion by fixing polynomial math.
    ([!666](https://gitlab.freedesktop.org/monado/monado/merge_requests/666))
  - u/distortion: Improve Index distortion and tidy code. While this touches the
    Vive distortion code all Vive headsets seems to have the center set to the same
    for each channel so doesn't help them. And Vive doesn't have the extra
    coefficient that the Index does so no help there either.
    ([!667](https://gitlab.freedesktop.org/monado/monado/merge_requests/667))
- Misc. Features
  - Work toward a Win32 port.
    ([!551](https://gitlab.freedesktop.org/monado/monado/merge_requests/551),
    [!605](https://gitlab.freedesktop.org/monado/monado/merge_requests/605),
    [!607](https://gitlab.freedesktop.org/monado/monado/merge_requests/607))
  - Additional improvements to the Android port.
    ([!592](https://gitlab.freedesktop.org/monado/monado/merge_requests/592),
    [!595](https://gitlab.freedesktop.org/monado/monado/merge_requests/595),
    [#105](https://gitlab.freedesktop.org/monado/monado/issues/105))
- Misc. Fixes
  - steamvr: Support HMDs with rotated displays
    ([!600](https://gitlab.freedesktop.org/monado/monado/merge_requests/600))

## Monado 0.4.1 (2020-11-04)

- State Trackers
  - st/oxr: Fix for new conformance tests for xrWaitFrame, xrBeginFrame,
    xrEndFrame call order. Also fix OpenXR state transition logic depending on a
    synchronized frame loop.
    ([!589](https://gitlab.freedesktop.org/monado/monado/merge_requests/589),
    [!590](https://gitlab.freedesktop.org/monado/monado/merge_requests/590))

## Monado 0.4.0 (2020-11-02)

- XRT Interface
  - add `xrt_device_type` to `xrt_device` to differentiate handed controllers
    from
    controllers that can be held in either hand.
    ([!412](https://gitlab.freedesktop.org/monado/monado/merge_requests/412))
  - Rename functions and types that assumed the native graphics buffer handle type
    was an FD: in `auxiliary/vk/vk_helpers.{h,c}` `vk_create_image_from_fd` ->
    `vk_create_image_from_native`, in the XRT headers `struct xrt_compositor_fd` ->
    `xrt_compositor_native` (and method name changes from `xrt_comp_fd_...` ->
    `xrt_comp_native_...`), `struct xrt_swapchain_fd` -> `struct
    xrt_swapchain_native`, `struct xrt_image_fd` -> `struct xrt_image_native`, and
    corresponding parameter/member/variable name changes (e.g. `struct
    xrt_swapchain_fd *xscfd` becomes `struct xrt_swapchain_native *xscn`).
    ([!426](https://gitlab.freedesktop.org/monado/monado/merge_requests/426),
    [!428](https://gitlab.freedesktop.org/monado/monado/merge_requests/428))
  - Make some fields on `xrt_gl_swapchain` and `xrt_vk_swapchain` private moving
    them into the client compositor code instead of exposing them.
    ([!444](https://gitlab.freedesktop.org/monado/monado/merge_requests/444))
  - Make `xrt_compositor::create_swapchain` return xrt_result_t instead of the
    swapchain, this makes the methods on `xrt_compositor` more uniform.
    ([!444](https://gitlab.freedesktop.org/monado/monado/merge_requests/444))
  - Add the method `xrt_compositor::import_swapchain` allowing a state tracker to
    create a swapchain from a set of pre-allocated images. Uses the same
    `xrt_swapchain_create_info` as `xrt_compositor::create_swapchain`.
    ([!444](https://gitlab.freedesktop.org/monado/monado/merge_requests/444))
  - Make `xrt_swapchain_create_flags` swapchain static image bit match OpenXR.
    ([!454](https://gitlab.freedesktop.org/monado/monado/merge_requests/454))
  - Add `XRT_SWAPCHAIN_USAGE_INPUT_ATTACHMENT` flag to `xrt_swapchain_usage_bits`
    so that a client can create a Vulkan swapchain that can be used as input
    attachment.
    ([!459](https://gitlab.freedesktop.org/monado/monado/merge_requests/459))
  - Remove the `flip_y` parameter to the creation of the native compositor, this
    is
    now a per layer thing.
    ([!461](https://gitlab.freedesktop.org/monado/monado/merge_requests/461))
  - Add `xrt_compositor_info` struct that allows the compositor carry information
    to about its capabilities and its recommended values. Not everything is hooked
    up at the moment.
    ([!461](https://gitlab.freedesktop.org/monado/monado/merge_requests/461))
  - Add defines for underlying handle types.
    ([!469](https://gitlab.freedesktop.org/monado/monado/merge_requests/469))
  - Add a native handle type for graphics sync primitives (currently file
    descriptors on all platforms).
    ([!469](https://gitlab.freedesktop.org/monado/monado/merge_requests/469))
  - Add a whole bunch of structs and functions for all of the different layers
    in
    OpenXR. The depth layer information only applies to the stereo projection
    so
    make a special stereo projection with depth layer.
    ([!476](https://gitlab.freedesktop.org/monado/monado/merge_requests/476))
  - Add `xrt_image_native_allocator` as a friend to the compositor interface. This
    simple interface is intended to be used by the IPC interface to allocate
    `xrt_image_native` on the client side and send those to the service.
    ([!478](https://gitlab.freedesktop.org/monado/monado/merge_requests/478))
  - Re-arrange and document `xrt_image_native`, making the `size` field optional.
    ([!493](https://gitlab.freedesktop.org/monado/monado/merge_requests/493))
  - Add const to all compositor arguments that are info structs, making the
    interface safer and
    more clear. Also add `max_layers` field to the
    `xrt_compositor_info` struct.
    ([!501](https://gitlab.freedesktop.org/monado/monado/merge_requests/501))
  - Add `xrt_space_graph` struct for calculating space relations. This struct and
    accompanying makes it easier to reason about space relations than just
    functions
    operating directly on `xrt_space_relation`. The code base is changed
    to use
    these new functions.
    ([!519](https://gitlab.freedesktop.org/monado/monado/merge_requests/519))
  - Remove the `linear_acceleration` and `angular_acceleration` fields from the
    `xrt_space_relation` struct, these were not used in the codebase and are not
    exposed in the OpenXR API. They can easily be added back should they be
    required again by code or a future feature. Drivers are free to retain this
    information internally, but no longer expose it.
    ([!519](https://gitlab.freedesktop.org/monado/monado/merge_requests/519))
  - Remove the `out_timestamp` argument to the `xrt_device::get_tracked_pose`
    function, it's not needed anymore and the devices can do prediction better
    as
    it knows more about its tracking system the the state tracker.
    ([!521](https://gitlab.freedesktop.org/monado/monado/merge_requests/521))
  - Replace mesh generator with `compute_distortion` function on `xrt_device`. This
    is used to both make it possible to use mesh shaders for devices and to provide
    compatibility with SteamVR which requires a `compute_distortion` function as
    well.

    The compositor uses this function automatically to create a mesh and
    uses mesh
    distortion for all drivers. The function `compute_distortion` default
    implementations for `none`, `panotools` and `vive` distortion models are
    provided in util.
    ([!536](https://gitlab.freedesktop.org/monado/monado/merge_requests/536))
  - Add a simple curl value based finger tracking model and use it for vive and
    survive controllers.
    ([!555](https://gitlab.freedesktop.org/monado/monado/merge_requests/555))
- State Trackers
  - OpenXR: Add support for attaching Quad layers to action sapces.
    ([!437](https://gitlab.freedesktop.org/monado/monado/merge_requests/437))
  - OpenXR: Use initial head pose as origin for local space.
    ([!443](https://gitlab.freedesktop.org/monado/monado/merge_requests/443))
  - OpenXR: Minor fixes for various bits of code: copy-typo in device assignment
    code; better stub for the unimplemented function
    `xrEnumerateBoundSourcesForAction`; better error message on internal error in
    `xrGetCurrentInteractionProfile`.
    ([!448](https://gitlab.freedesktop.org/monado/monado/merge_requests/448))
  - OpenXR: Make the `xrGetCurrentInteractionProfile` conformance tests pass,
    needed
    to implement better error checking as well as generating
    `XrEventDataInteractionProfileChanged` events to the client.
    ([!448](https://gitlab.freedesktop.org/monado/monado/merge_requests/448))
  - OpenXR: Centralize all sub-action path iteration in some x-macros.
    ([!449](https://gitlab.freedesktop.org/monado/monado/merge_requests/449),
    [!456](https://gitlab.freedesktop.org/monado/monado/merge_requests/456))
  - OpenXR: Improve the validation in the API function for
    `xrGetInputSourceLocalizedName`.
    ([!451](https://gitlab.freedesktop.org/monado/monado/merge_requests/451))
  - OpenXR: Implement the function `xrEnumerateBoundSourcesForAction`, currently we
    only bind one input per top level user path and it's easy to track this.
    ([!451](https://gitlab.freedesktop.org/monado/monado/merge_requests/451))
  - OpenXR: Properly handle more than one input source being bound to the same
    action
    according to the combination rules of the specification.
    ([!452](https://gitlab.freedesktop.org/monado/monado/merge_requests/452))
  - OpenXR: Fix multiplicity of bounds paths per action - there's one per
    input/output.
    ([!456](https://gitlab.freedesktop.org/monado/monado/merge_requests/456))
  - OpenXR: Implement the MND_swapchain_usage_input_attachment_bit extension.
    ([!459](https://gitlab.freedesktop.org/monado/monado/merge_requests/459))
  - OpenXR: Refactor the native compositor handling a bit, this creates the
    compositor earlier then before. This allows us to get the viewport information
    from it.
    ([!461](https://gitlab.freedesktop.org/monado/monado/merge_requests/461))
  - OpenXR: Implement action set priorities and fix remaining action conformance
    tests.
    ([!462](https://gitlab.freedesktop.org/monado/monado/merge_requests/462))
  - st/oxr: Fix crash when calling `xrPollEvents` when headless mode is selected.
    ([!475](https://gitlab.freedesktop.org/monado/monado/merge_requests/475))
  - OpenXR: Add stub functions and support plumbing for a lot of layer extensions.
    ([!476](https://gitlab.freedesktop.org/monado/monado/merge_requests/476))
  - OpenXR: Be sure to return `XR_ERROR_FEATURE_UNSUPPORTED` if the protected
    content bit is set and the compositor does not support it.
    ([!481](https://gitlab.freedesktop.org/monado/monado/merge_requests/481))
  - OpenXR: Update to 1.0.11 and start returning the new
    `XR_ERROR_GRAPHICS_REQUIREMENTS_CALL_MISSING` code added in this release.
    ([!482](https://gitlab.freedesktop.org/monado/monado/merge_requests/482))
  - OpenXR: Enable the `XR_KHR_android_create_instance` extension.
    ([!492](https://gitlab.freedesktop.org/monado/monado/merge_requests/492))
  - OpenXR: Add support for creating swapchains with depth formats and submitting
    depth layers. The depth layers are passed through to the compositor, but are
    not used yet.
    ([!498](https://gitlab.freedesktop.org/monado/monado/merge_requests/498))
  - OpenXR: For pose actions the any path (`XR_NULL_PATH`) needs to be special
    cased, essentially turning into a separate action sub path, that is assigned
    at
    binding time.
    ([!510](https://gitlab.freedesktop.org/monado/monado/merge_requests/510))
  - OpenXR: More correctly implement `xrGetInputSourceLocalizedName` allowing apps
    to more accurently tell the user which input to use.
    ([!532](https://gitlab.freedesktop.org/monado/monado/merge_requests/532))
  - OpenXR: Pass through equirect layer data to the compositor.
    ([!566](https://gitlab.freedesktop.org/monado/monado/merge_requests/566))
- Drivers
  - psvr: We were sending in the wrong type of time to the 3DOF fusion code,
    switch
    to nanoseconds instead of fractions of seconds.
    ([!474](https://gitlab.freedesktop.org/monado/monado/merge_requests/474))
  - rs: Make the pose getting from the T265 be threaded. Before we where getting
    the
    pose from the update input function, this would cause some the main thread
    to
    block and would therefore cause jitter in the rendering.
    ([!486](https://gitlab.freedesktop.org/monado/monado/merge_requests/486))
  - survive: Add lighthouse tracking system type
    hydra: Add lighthouse tracking
    system type
    ([!489](https://gitlab.freedesktop.org/monado/monado/merge_requests/489))
  - rs: Add slam tracking system type
    northstar: Use tracking system from tracker
    (e.g. rs) if available.
    ([!494](https://gitlab.freedesktop.org/monado/monado/merge_requests/494))
  - psmv: Introduce proper grip and aim poses, correctly rotate the grip pose to
    follow the spec more closely. The aim poses replaces the previous ball tip pose
    that was used before for aim.
    ([!509](https://gitlab.freedesktop.org/monado/monado/merge_requests/509))
  - survive: Implement haptic feedback.
    ([!557](https://gitlab.freedesktop.org/monado/monado/merge_requests/557))
  - simulated: Tidy the code a bit and switch over to the new
    logging API.
    ([!572](https://gitlab.freedesktop.org/monado/monado/merge_requests/572),
    [!573](https://gitlab.freedesktop.org/monado/monado/merge_requests/573))
  - psvr: Switch to the new logging API.
    ([!573](https://gitlab.freedesktop.org/monado/monado/merge_requests/573))
  - Add initial "Cardboard" phone-holder driver for Android.
    ([!581](https://gitlab.freedesktop.org/monado/monado/merge_requests/581))
- IPC
  - Generalize handling of native-platform handles in IPC code, allow bi-
    directional handle transfer, and de-duplicate code between server and client.
    ([!413](https://gitlab.freedesktop.org/monado/monado/merge_requests/413),
    [!427](https://gitlab.freedesktop.org/monado/monado/merge_requests/427))
  - generation: Fix handling 'in_handle' by adding a extra sync round-trip, this
    might be solvable by using `SOCK_SEQPACKET`.
    ([!444](https://gitlab.freedesktop.org/monado/monado/merge_requests/444))
  - Implement the `xrt_compositor::import_swapchain` function, uses the earlier
    `in_handle` work.
    ([!444](https://gitlab.freedesktop.org/monado/monado/merge_requests/444))
  - proto: Transport the `xrt_compositor_info` over the wire so that the client can
    get the needed information.
    ([!461](https://gitlab.freedesktop.org/monado/monado/merge_requests/461))
  - client: Implement the usage of the `xrt_image_native_allocator`, currently not
    used. But it is needed for platforms where for various reasons the allocation
    must happen on the client side.
    ([!478](https://gitlab.freedesktop.org/monado/monado/merge_requests/478))
  - client: Add a "loopback" image allocator, this code allocates a swapchain from
    the service then imports that back to the service as if it was imported. This
    tests both the import code and the image allocator code.
    ([!478](https://gitlab.freedesktop.org/monado/monado/merge_requests/478))
  - ipc: Allow sending zero handles as a reply, at least the Linux fd handling code
    allows this.
    ([!491](https://gitlab.freedesktop.org/monado/monado/merge_requests/491))
  - Use a native AHardwareBuffer allocator on the client side when building for
    recent-enough Android.
    ([!493](https://gitlab.freedesktop.org/monado/monado/merge_requests/493))
  - ipc: Add functionality to disable a device input via the `monado-ctl` utility,
    this allows us to pass the conformance tests that requires the runtime to turn
    off a device.
    ([!511](https://gitlab.freedesktop.org/monado/monado/merge_requests/511))
- Compositor
  - compositor: Add support for alpha blending with premultiplied alpha.
    ([!425](https://gitlab.freedesktop.org/monado/monado/merge_requests/425))
  - compositor: Implement subimage rectangle rendering for quad layers.
    ([!433](https://gitlab.freedesktop.org/monado/monado/merge_requests/433))
  - compositor: Enable subimage rectangle rendering for projection layers.
    ([!436](https://gitlab.freedesktop.org/monado/monado/merge_requests/436))
  - compositor: Fix printing of current connected displays on nvidia when no
    allowed display is found.
    ([!477](https://gitlab.freedesktop.org/monado/monado/merge_requests/477))
  - compositor: Add env var to temporarily add display string to NVIDIA allowlist.
    ([!477](https://gitlab.freedesktop.org/monado/monado/merge_requests/477))
  - compositor and clients: Use a generic typedef to represent the platform-
    specific graphics buffer, allowing use of `AHardwareBuffer` on recent Android.
    ([!479](https://gitlab.freedesktop.org/monado/monado/merge_requests/479))
  - compositor: Check the protected content bit, and return a non-success code if
    it's set. Supporting this is optional in OpenXR, but lack of support must be
    reported to the application.
    ([!481](https://gitlab.freedesktop.org/monado/monado/merge_requests/481))
  - compositor: Implement cylinder layers.
    ([!495](https://gitlab.freedesktop.org/monado/monado/merge_requests/495))
  - main: Set the maximum layers supported to 16, we technically support more than
    16, but things get out of hand if multiple clients are running and all are
    using
    max layers.
    ([!501](https://gitlab.freedesktop.org/monado/monado/merge_requests/501))
  - main: Add code to check that a format is supported by the GPU before exposing.
    ([!502](https://gitlab.freedesktop.org/monado/monado/merge_requests/502))
  - compositor: Remove panotools and vive shaders from compositor.
    ([!538](https://gitlab.freedesktop.org/monado/monado/merge_requests/538))
  - Initial work on a port of the compositor to Android.
    ([!547](https://gitlab.freedesktop.org/monado/monado/merge_requests/547))
  - render: Implement equirect layer rendering.
    ([!566](https://gitlab.freedesktop.org/monado/monado/merge_requests/566))
  - main: Fix leaks of sampler objects that was introduced in !566.
    ([!571](https://gitlab.freedesktop.org/monado/monado/merge_requests/571))
- Helper Libraries
  - u/vk: Remove unused vk_image struct, this is later recreated for the image
    allocator code.
    ([!444](https://gitlab.freedesktop.org/monado/monado/merge_requests/444))
  - u/vk: Add a new image allocate helper, this is used by the main compositor to
    create, export and import swapchain images.
    ([!444](https://gitlab.freedesktop.org/monado/monado/merge_requests/444))
  - u/vk: Rename `vk_create_semaphore_from_fd` to `vk_create_semaphore_from_native`
    ([!469](https://gitlab.freedesktop.org/monado/monado/merge_requests/469))
  - aux/android: New Android utility library added.
    ([!493](https://gitlab.freedesktop.org/monado/monado/merge_requests/493),
    [!547](https://gitlab.freedesktop.org/monado/monado/merge_requests/547),
    [!581](https://gitlab.freedesktop.org/monado/monado/merge_requests/581))
  - aux/ogl: Add a function to compute the texture target and binding enum for a
    given swapchain image creation info.
    ([!493](https://gitlab.freedesktop.org/monado/monado/merge_requests/493))
  - util: Tidy hand tracking header.
    ([!574](https://gitlab.freedesktop.org/monado/monado/merge_requests/574))
  - math: Fix doxygen warnings in vector headers.
    ([!574](https://gitlab.freedesktop.org/monado/monado/merge_requests/574))
- Misc. Features
  - Support building in-process Monado with meson.
    ([!421](https://gitlab.freedesktop.org/monado/monado/merge_requests/421))
  - Allow building some components without Vulkan. Vulkan is still required for the
    compositor and therefore the OpenXR runtime target.
    ([!429](https://gitlab.freedesktop.org/monado/monado/merge_requests/429))
  - Add an OpenXR Android target: an APK which provides an "About" activity and
    eventually, an OpenXR runtime.
    ([!574](https://gitlab.freedesktop.org/monado/monado/merge_requests/574),
    [!581](https://gitlab.freedesktop.org/monado/monado/merge_requests/581))
- Misc. Fixes
  - No significant changes

## Monado 0.3.0 (2020-07-10)

- Major changes
  - Centralise the logging functionality in Monado to a single util helper.
    Previously most of our logging was done via fprints and gated behind booleans,
    now there are common functions to call and a predefined set of levels.
    ([!408](https://gitlab.freedesktop.org/monado/monado/merge_requests/408),
    [!409](https://gitlab.freedesktop.org/monado/monado/merge_requests/409))
- XRT Interface
  - compositor: Remove the `array_size` field from the struct, this was the only
    state tracker supplied value that was on the struct, only have values that the
    compositor decides over on the struct.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - Improve Doxygen documentation of interfaces. Now the inheritance structure and
    implementation of interfaces is shown in the docs, and helper functions that
    call through function pointers are listed as "member functions", to help
    developers understand the internal structure of Monado better.
    ([!365](https://gitlab.freedesktop.org/monado/monado/merge_requests/365),
    [!367](https://gitlab.freedesktop.org/monado/monado/merge_requests/367))
  - xrt: Add xrt_result_t return type to many compositor functions that previously
    had no way to indicate failure.
    ([!369](https://gitlab.freedesktop.org/monado/monado/merge_requests/369))
  - compositor: Introduce `xrt_swapchain_create_info` simplifying the argument
    passing between various layers of the compositor stack and also simplify future
    refactoring projects.
    ([!407](https://gitlab.freedesktop.org/monado/monado/merge_requests/407))
- State Trackers
  - OpenXR: Update headers to 1.0.9.
    ([!358](https://gitlab.freedesktop.org/monado/monado/merge_requests/358))
  - OpenXR: Verify that the XrViewConfigurationType is supported by the system as
    required by the OpenXR spec in xrEnumerateEnvironmentBlendModes.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Return the correct error code when verifying the sub action, if it is
    a
    valid sub action path but not given at action creation we should return
    `XR_ERROR_PATH_UNSUPPORTED`.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Validate the subImage data for both projection and quad layers layers,
    refactor code out so it can be shared with the different types of layers. Need
    to track some state on the `oxr_swapchain` in order to do the checking.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Correct the return error code for action and action set localized name
    validation.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Correct the error messages on sub action paths errors.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Track the name and localized name for both actions and action sets,
    that
    way we can make sure that there are no duplicates. This is required by the
    spec. ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Do better checking if action sets and actions have been attached to the
    session or not.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Validate the arguments for `xrSuggestInteractionProfileBindings` better
    so that it follows the spec better.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Rework the logging formatting of error messages, this makes it easier
    to
    read for the application developer.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Correctly ensure that the application has called the required get
    graphics requirements function when creating a session.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: When an `XrSession` is destroyed purge the event queue of any events
    that
    references to it so that no events gets delivered to the applications with
    stales handles.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Make the event queue thread safe, all done with a simple mutex that is
    not held for long at all.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: A major overhaul of the swapchain acquire, wait and release code. This
    makes it almost completely conformant with the spec. Tricky parts include that
    multiple images can be acquired, but only one can be waited on before being
    released.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Enforce that static swapchains can only be acquired once, this is
    required by the spec and make sure that a image is only rendered to once, and
    allows the runtime to perform special optimizations on the image.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Make the function `xrGetReferenceSpaceBoundsRect` at least conform to
    the spec without actually implementing it, currently we do not track bounds in
    Monado.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Make the session state changes obey the specification. The code is
    fairly hair as is and should be improved at a later time.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Use the correct XrPath for `/user/gamepad` while it sits in the users
    hand itsn't `/user/hand/gamepad` as previously believed.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - OpenXR: Where used make sure we verify the view configuration type is a valid
    enum value, the code is setup so that we in the future can support new values
    via extensions easily.
    ([!368](https://gitlab.freedesktop.org/monado/monado/merge_requests/368))
  - OpenXR: More correctly verify the interactive profile binding data, including
    the given interactive profile is correct and the binding point is valid.
    ([!377](https://gitlab.freedesktop.org/monado/monado/merge_requests/377))
  - OpenXR: Transform input types in a somewhat flexible, composable way. Also, do
    conversion at sync time, and use the transformed values to evaluate if the
    input has changed, per the spec.
    ([!379](https://gitlab.freedesktop.org/monado/monado/merge_requests/379))
  - OpenXR: Tidy the extensions generated by the script and order them according
    to
    extension prefix, starting with KHR, EXT, Vendor, KHRX, EXTX, VendorX. Also
    rename the `MND_ball_on_stick_controller` to `MNDX_ball_on_a_stick_controller`.
    ([!410](https://gitlab.freedesktop.org/monado/monado/merge_requests/410))
  - OpenXR: Fix overly attached action sets, which would appear to be attached to
    a
    session even after the session has been destroyed. Also tidy up comments and
    other logic surrounding this.
    ([!411](https://gitlab.freedesktop.org/monado/monado/merge_requests/411))
- Drivers
  - psvr: Normalize the rotation to not trip up the client app when it gives the
    rotation back to `st/oxr` again.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - vive: Create vive_config module to isolate config code and avoid duplication
    between controller and headset code.
    vive: Probe for controllers in vive_proper
    interface.
    vive: Fix a bug where using the Vive Pro crashed Monado.
    vive: Fix a
    bug where the controller didn't parse JSON vectors correctly.
    vive: Move
    missing functions to and use u_json.
    ([!405](https://gitlab.freedesktop.org/monado/monado/merge_requests/405))
  - vive: Add support for Gen1 and Gen2 Vive Trackers.
    ([!406](https://gitlab.freedesktop.org/monado/monado/merge_requests/406))
  - vive: Port to new u_logging API.
    ([!417](https://gitlab.freedesktop.org/monado/monado/merge_requests/417))
  - comp: Set a compositor window title.
    ([!418](https://gitlab.freedesktop.org/monado/monado/merge_requests/418))
- IPC
  - server: Almost completely overhaul the handling of swapchain lifecycle
    including: correctly track which swapchains are alive; reuse ids; enforce the
    maximum number of swapchains; and destroy underlying swapchains when they are
    destroyed by the client.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - util: Make sure to not dereference NULL control messages, say in the case of the
    server failing to create a swapchain. Also add a whole bunch of paranoia when
    it comes to the alignment of the control message buffers.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - ipc: Return XR_ERROR_INSTANCE_LOST on IPC errors.
    ([!369](https://gitlab.freedesktop.org/monado/monado/merge_requests/369))
- Compositor
  - main: Include `<math.h>` in layers renderer for missing `tanf` function.
    ([!358](https://gitlab.freedesktop.org/monado/monado/merge_requests/358))
  - swapchain: Give out the oldset image index when a image is acquired. This logic
    can be made better, but will work for the good case.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - swapchain: Close any FDs that are still valid, for instance the ipc server
    copies the FDs to the client.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - client: When we give a image fd to the either OpenGL or Vulkan it is consumed
    and cannot be reused. So make sure that it is set to an invalid fd value on the
    `xrt_image_fd` on the owning `xrt_swapchain_fd`.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - main: We were alpha blending all layers previously, but now we support the
    layer flag that OpenXR gives us. We do this by using different `VkImageView`s
    with different component swizzles.
    ([!394](https://gitlab.freedesktop.org/monado/monado/merge_requests/394))
  - layer_rendering: Use the visibility flags on quad to correctly show the layers
    in each eye.
    ([!394](https://gitlab.freedesktop.org/monado/monado/merge_requests/394))
- Helper Libraries
  - os/threading: Include `xrt_compiler.h` to fix missing stdint types.
    ([!358](https://gitlab.freedesktop.org/monado/monado/merge_requests/358))
  - util: Add a very simple fifo for indices, this is used to keep track of
    swapchain in order of age (oldness).
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
  - util: Expand `u_hashset` to be able to automatically allocate a `u_hashet_item`
    and insert it.
    ([!359](https://gitlab.freedesktop.org/monado/monado/merge_requests/359))
- Misc. Features
  - build: Allow enabling inter-procedural optimization in CMake GUIs, if supported
    by platform and compiler.
    ([!330](https://gitlab.freedesktop.org/monado/monado/merge_requests/330))
- Misc. Fixes
  - No significant changes

## Monado 0.2 (2020-05-29)

- Major changes
  - Add support for a new service process. This process houses the hardware drivers
    and compositor. In order to do this, a whole new subsection of Monado called
    ipc
    was added. It lives in `src/xrt/ipc` and sits between the state trackers
    and
    the service hosting the drivers and compositor.
    ([!295](https://gitlab.freedesktop.org/monado/monado/merge_requests/295))
  - Support optional systemd socket-activation: if not disabled at configure time,
    `monado-service` can be launched by systemd as a service with an associated
    socket. If the service is launched this way, it will use the systemd-created
    domain socket instead of creating its own. (If launched manually, it will still
    create its own as normal.) This allows optional auto-launching of the service
    when running a client (OpenXR) application. Associated systemd unit files are
    also included.
    ([!306](https://gitlab.freedesktop.org/monado/monado/merge_requests/306))
- XRT Interface
  - Add a new settings interface for transporting camera settings, in
    `xrt/xrt_settings.h`.
    ([!266](https://gitlab.freedesktop.org/monado/monado/merge_requests/266))
  - Make it possible to send JSON object to drivers when probing for devices.
    ([!266](https://gitlab.freedesktop.org/monado/monado/merge_requests/266))
  - Added new `xrt_instance` object to be root object, a singleton that allows us
    to
    better swap out the whole stack underneath the state trackers. This is now
    implemented by the `xrt_prober` code and used by the OpenXR state tracker.
    ([!274](https://gitlab.freedesktop.org/monado/monado/merge_requests/274))
  - Remove the `struct timestate` argument from the `struct xrt_device` interface.
    It should be easy to write a driver and the state tracker should be the one
    that tracks this state. It was mostly triggered by the out of process
    compositor work.
    ([!280](https://gitlab.freedesktop.org/monado/monado/merge_requests/280))
  - Add the new format `XRT_FORMAT_UYVY422` to the interface and various parts of
    the code where it is needed to be supported, like the conversion functions and
    calibration code. Also rename the `XRT_FORMAT_YUV422` to `XRT_FORMAT_YUYV422`.
    ([!283](https://gitlab.freedesktop.org/monado/monado/merge_requests/283))
  - Expose manufacturer and serial number in the prober interface, right now only
    for the video device probing. But this is the only thing that really requires
    this in order to tell different cameras apart.
    ([!286](https://gitlab.freedesktop.org/monado/monado/merge_requests/286))
  - Add `XRT_CAST_PTR_TO_OXR_HANDLE` and `XRT_CAST_OXR_HANDLE_TO_PTR` macros to
    perform warning-free conversion between pointers and OpenXR handles, even on
    32-bit platforms. They should be used instead of raw casts.
    ([!294](https://gitlab.freedesktop.org/monado/monado/merge_requests/294))
  - Remove declaration and implementations of `xrt_prober_create`: the minimal
    functionality previously performed there should now be moved to
    `xrt_instance_create`.
    ([!347](https://gitlab.freedesktop.org/monado/monado/merge_requests/347))
- State Trackers
  - gui: Fix compilation issue in `st/gui` when building without OpenCV.
    ([#63](https://gitlab.freedesktop.org/monado/monado/issues/63),
    [!256](https://gitlab.freedesktop.org/monado/monado/merge_requests/256))
  - OpenXR: Don't return struct with invalid type from
    `xrEnumerateViewConfigurationViews`.
    ([!234](https://gitlab.freedesktop.org/monado/monado/merge_requests/234))
  - prober: Print more information from the prober when spewing.
    ([!261](https://gitlab.freedesktop.org/monado/monado/merge_requests/261))
  - gui: Save camera and calibration data using new settings structs and format.
    ([!266](https://gitlab.freedesktop.org/monado/monado/merge_requests/266))
  - prober: Load tracking config from json and use new settings struct.
    ([!266](https://gitlab.freedesktop.org/monado/monado/merge_requests/266))
  - gui: Fix name not being shown when video device does not have any modes.
    ([!269](https://gitlab.freedesktop.org/monado/monado/merge_requests/269))
  - gui: Remove old video test scene, never used and seemed to be broken.
    ([!275](https://gitlab.freedesktop.org/monado/monado/merge_requests/275))
  - gui: Fix build when OpenCV is not available or disabled.
    ([!292](https://gitlab.freedesktop.org/monado/monado/merge_requests/292))
  - OpenXR: Fix build when OpenGL is not enabled.
    ([!292](https://gitlab.freedesktop.org/monado/monado/merge_requests/292))
  - OpenXR: Validate that we support the given `XR_ENVIRONMENT_BLEND_MODE` as
    according to the OpenXR spec. And better print the error messages.
    ([!345](https://gitlab.freedesktop.org/monado/monado/merge_requests/345))
  - OpenXR: Validate given `displayTime` in `xrEndFrame` as required by the spec.
    ([!345](https://gitlab.freedesktop.org/monado/monado/merge_requests/345))
  - OpenXR: Validate internal state that we get from the compositor.
    ([!345](https://gitlab.freedesktop.org/monado/monado/merge_requests/345))
  - OpenXR: Validate time better in xrConvertTimeToTimespecTimeKHR and add better
    error print.
    ([!348](https://gitlab.freedesktop.org/monado/monado/merge_requests/348))
  - OpenXR: Correctly translate the `XrSwapchainCreateFlags` flags to xrt ones.
    ([!349](https://gitlab.freedesktop.org/monado/monado/merge_requests/349))
  - OpenXR: In order to be able to correctly validate `XrPath` ids turn them
    into a atom and keep all created paths in a array.
    ([!349](https://gitlab.freedesktop.org/monado/monado/merge_requests/349))
  - OpenXR: Give better error messages on invalid poses in quad layers instead of
    using the simple macro.
    ([!350](https://gitlab.freedesktop.org/monado/monado/merge_requests/350))
  - OpenXR: Validate poses for project layer views, using the same expressive error
    messages as the quad layers.
    ([!350](https://gitlab.freedesktop.org/monado/monado/merge_requests/350))
  - OpenXR: Translate the swapchain usage bits from OpenXR enums to Monado's
    internal enums.
    ([!350](https://gitlab.freedesktop.org/monado/monado/merge_requests/350))
  - OpenXR: Report a spec following amount of maximum layers supported.
    ([!354](https://gitlab.freedesktop.org/monado/monado/merge_requests/354))
  - OpenXR: Correctly reject invalid times given to `xrLocateSpace`.
    ([!354](https://gitlab.freedesktop.org/monado/monado/merge_requests/354))
  - OpenXR: Correctly handle the space relation flag bits, some old hacked up code
    left over since Monado's first days have been removed.
    ([!356](https://gitlab.freedesktop.org/monado/monado/merge_requests/356))
- Drivers
  - dd: Add a driver for the Google Daydream View controller.
    ([!242](https://gitlab.freedesktop.org/monado/monado/merge_requests/242))
  - all: Use new pre-filter and 3-DoF filter in drivers.
    ([!249](https://gitlab.freedesktop.org/monado/monado/merge_requests/249))
  - arduino: Added a Arduino based flexible input device driver, along with
    Arduino C++ code for it.
    ([!251](https://gitlab.freedesktop.org/monado/monado/merge_requests/251))
  - psmv: Use all 6 measurements to compute acceleration bias, and port to new
    IMU prefilter.
    ([!255](https://gitlab.freedesktop.org/monado/monado/merge_requests/255))
  - v4l2: Add special tweaks for the ELP camera.
    ([!266](https://gitlab.freedesktop.org/monado/monado/merge_requests/266))
  - vive: Add basic 3DOF driver for Vive Wand Controller with full input support
    and
    Valve Index Controller with partial input support.
    ([!281](https://gitlab.freedesktop.org/monado/monado/merge_requests/281))
  - psvr: Use a better 3dof fusion for the PSVR when no tracking is available.
    ([!282](https://gitlab.freedesktop.org/monado/monado/merge_requests/282))
  - psvm: Move the led and rumble updating from the application facing
    update_inputs
    function to the internal thread instead.
    ([!287](https://gitlab.freedesktop.org/monado/monado/merge_requests/287))
  - psmv: Fix failure to build from source on PPC.
    ([!288](https://gitlab.freedesktop.org/monado/monado/merge_requests/288),
    [#69](https://gitlab.freedesktop.org/monado/monado/issues/69))
- Compositor
  - main: Fix XCB memory leaks and correctly use XCB/Xlib interop.
    ([!257](https://gitlab.freedesktop.org/monado/monado/merge_requests/257))
  - main: Shorten Vulkan initializers.
    ([!259](https://gitlab.freedesktop.org/monado/monado/merge_requests/259))
  - main: Port XCB and direct mode back ends to plain C.
    ([!262](https://gitlab.freedesktop.org/monado/monado/merge_requests/262))
  - main: Add support for Vive Pro, Valve Index, Oculus DK1, DK2 and CV1 to NVIDIA
    direct mode.
    ([!263](https://gitlab.freedesktop.org/monado/monado/merge_requests/263))
  - client: Make sure that the number of images is decided by the fd compositor.
    ([!270](https://gitlab.freedesktop.org/monado/monado/merge_requests/270))
  - main: Split RandR and NVIDIA direct mode window back ends.
    ([!271](https://gitlab.freedesktop.org/monado/monado/merge_requests/271))
  - main: Improve synchronization and remove redundant vkDeviceWaitIdle calls.
    ([!277](https://gitlab.freedesktop.org/monado/monado/merge_requests/277))
  - main: Delay the destruction of swapchains until a time where it is safe, this
    allows swapchains to be destroyed from other threads.
    ([!278](https://gitlab.freedesktop.org/monado/monado/merge_requests/278))
  - client: Propagate the supported formats from the real compositor to the client
    ones. ([!282](https://gitlab.freedesktop.org/monado/monado/merge_requests/282))
  - renderer: Change the idle images colour from bright white to grey.
    ([!282](https://gitlab.freedesktop.org/monado/monado/merge_requests/282))
  - main: Add support for multiple projection layers.
    ([!340](https://gitlab.freedesktop.org/monado/monado/merge_requests/340))
  - main: Implement quad layers.
    ([!340](https://gitlab.freedesktop.org/monado/monado/merge_requests/340))
  - main: Only allocate one image for static swapchains.
    ([!349](https://gitlab.freedesktop.org/monado/monado/merge_requests/349))
- Helper Libraries
  - tracking: Add image undistort/normalize cache mechanism, to avoid needing to
    remap every frame.
    ([!255](https://gitlab.freedesktop.org/monado/monado/merge_requests/255))
  - tracking: Improve readability and documentation of IMU fusion class.
    ([!255](https://gitlab.freedesktop.org/monado/monado/merge_requests/255))
  - u/file: Add file helpers to load files from config directory.
    ([!266](https://gitlab.freedesktop.org/monado/monado/merge_requests/266))
  - u/json: Add bool getter function.
    ([!266](https://gitlab.freedesktop.org/monado/monado/merge_requests/266))
  - tracking: Expose save function with non-hardcoded path for calibration data.
    ([!266](https://gitlab.freedesktop.org/monado/monado/merge_requests/266))
  - tracking: Remove all path hardcoded calibration data loading and saving
    functions.
    ([!266](https://gitlab.freedesktop.org/monado/monado/merge_requests/266))
  - threading: New helper functions and structs for doing threaded work, these are
    on a higher level then the one in os wrappers.
    ([!278](https://gitlab.freedesktop.org/monado/monado/merge_requests/278))
  - threading: Fix missing `#``pragma once` in `os/os_threading.h`.
    ([!282](https://gitlab.freedesktop.org/monado/monado/merge_requests/282))
  - u/time: Temporarily disable the time skew in time state and used fixed offset
    instead to fix various time issues in `st/oxr`. Will be fixed properly later.
    ([!348](https://gitlab.freedesktop.org/monado/monado/merge_requests/348))
  - math: Correctly validate quaternion using non-squared "length" instead of
    squared "length", certain combinations of elements would produce valid regular
    "length" but not valid squared ones.
    ([!350](https://gitlab.freedesktop.org/monado/monado/merge_requests/350))
- Misc. Features
  - build: Refactor CMake build system to make static (not object) libraries and
    explicitly describe dependencies.
    ([!233](https://gitlab.freedesktop.org/monado/monado/merge_requests/233),
    [!237](https://gitlab.freedesktop.org/monado/monado/merge_requests/237),
    [!238](https://gitlab.freedesktop.org/monado/monado/merge_requests/238),
    [!240](https://gitlab.freedesktop.org/monado/monado/merge_requests/240))
  - os/ble: Add utility functionality for accessing Bluetooth Low-Energy (Bluetooth
    LE or BLE) over D-Bus, in `os/os_ble.h` and `os/os_ble_dbus.c`.
    ([!242](https://gitlab.freedesktop.org/monado/monado/merge_requests/242))
  - util: Add some bit manipulation helper functions in `util/u_bitwise.c` and
    `util/u_bitwise.c`.
    ([!242](https://gitlab.freedesktop.org/monado/monado/merge_requests/242))
  - tracking: Make stereo_camera_calibration reference counted, and have the
    prober,
    not the calibration, call the save function.
    ([!245](https://gitlab.freedesktop.org/monado/monado/merge_requests/245))
  - math: Expand algebraic math functions in `math/m_api.h`, `math/m_vec3.h` and
    `math/m_base.cpp`.
    ([!249](https://gitlab.freedesktop.org/monado/monado/merge_requests/249))
  - math: Add pre-filter and a simple understandable 3-DoF fusion filter.
    ([!249](https://gitlab.freedesktop.org/monado/monado/merge_requests/249))
  - build: Enable the build system to install `monado-cli` and `monado-gui`.
    ([!252](https://gitlab.freedesktop.org/monado/monado/merge_requests/252))
  - build: Unify inputs for generated files between CMake and Meson builds.
    ([!252](https://gitlab.freedesktop.org/monado/monado/merge_requests/252))
  - build: Support building with system cJSON instead of bundled copy.
    ([!284](https://gitlab.freedesktop.org/monado/monado/merge_requests/284),
    [#62](https://gitlab.freedesktop.org/monado/monado/issues/62))
  - ci: Perform test builds using the Android NDK (for armeabi-v7a and armv8-a).
    This is not a full Android port (missing a compositor, etc) but it ensures we
    don't add more Android porting problems.
    ([!292](https://gitlab.freedesktop.org/monado/monado/merge_requests/292))
- Misc. Fixes
  - os/ble: Check if `org.bluez` name is available before calling in
    `os/os_ble_dbus.c`.
    ([#65](https://gitlab.freedesktop.org/monado/monado/issues/65),
    [#64](https://gitlab.freedesktop.org/monado/monado/issues/64),
    [!265](https://gitlab.freedesktop.org/monado/monado/merge_requests/265))
  - README: Added information to the README containing OpenHMD version requirement
    and information regarding the requirement of `GL_EXT_memory_object_fd` and
    limitations on Monado's compositor.
    ([!4](https://gitlab.freedesktop.org/monado/monado/merge_requests/4))
  - build: Fix build issues and build warnings when 32-bit.
    ([!230](https://gitlab.freedesktop.org/monado/monado/merge_requests/230))
  - os/ble: Fix crash due to bad dbus path, triggered by bad return checking when
    probing for BLE devices.
    ([!247](https://gitlab.freedesktop.org/monado/monado/merge_requests/247))
  - d/dd: Use the correct time delta in DayDream driver.
    ([!249](https://gitlab.freedesktop.org/monado/monado/merge_requests/249))
  - doc: Stop changelog snippets from showing up in 'Related Pages'
    ([!253](https://gitlab.freedesktop.org/monado/monado/merge_requests/253))
  - build: Fix meson warnings, increase compiler warning level.
    ([!258](https://gitlab.freedesktop.org/monado/monado/merge_requests/258))
  - os/ble: Fix leak in `os/os_ble_dbus.c` code when failing to find any device.
    ([!264](https://gitlab.freedesktop.org/monado/monado/merge_requests/264))
  - os/ble: Make ble code check for some error returns in `os/os_ble_dbus.c`.
    ([!265](https://gitlab.freedesktop.org/monado/monado/merge_requests/265))
  - u/hashset: Fix warnings in `util/u_hashset.h` after pedantic warnings were
    enabled for C++.
    ([!268](https://gitlab.freedesktop.org/monado/monado/merge_requests/268))
  - build: Fix failure to build from source on ppc64 and s390x.
    ([!284](https://gitlab.freedesktop.org/monado/monado/merge_requests/284))
  - build: Mark OpenXR runtime target in CMake as a MODULE library, instead of a
    SHARED library.
    ([!284](https://gitlab.freedesktop.org/monado/monado/merge_requests/284))
  - windows: Way way back when Gallium was made `auxiliary` was named `aux` but
    then
    it was ported to Windows and it was renamed to `auxiliary` since Windows
    is [allergic to filenames that match its device names](https://docs.microsoft.com/en-us/windows/win32/fileio/naming-a-file#naming-conventions)
    (e.g., `AUX`, `CON`, `PRN`, etc.). Through the ages, this knowledge was lost
    and so we find ourselves with the same problem. Although Monado inherited
    the correct name, the same old mistake was made in docs.
    ([!314](https://gitlab.freedesktop.org/monado/monado/merge_requests/314))
  - build: For CMake rename (nearly) all build options so they begin with `XRT_`
    and match the defines used in the source. You will probably want to clear
    your build directory and reconfigure from scratch.
    ([!327](https://gitlab.freedesktop.org/monado/monado/merge_requests/327))
  - ipc: Correctly set the shared semaphore value when creating it, the wrong value
    resulted in the client not blocking in `xrWaitFrame`.
    ([!348](https://gitlab.freedesktop.org/monado/monado/merge_requests/348))
  - ipc: Previously some arguments where dropped at swapchain creation time,
    correct pass them between the client and server.
    ([!349](https://gitlab.freedesktop.org/monado/monado/merge_requests/349))

## Monado 0.1.0 (2020-02-24)

Initial (non-release) tag to promote/support packaging.

