- xrt_defines: Added new define `XRT_MAX_VIEWS` to define the maximum number of views supported by the system. This is used to define the maximum number of views supported by the distortion calculation as well as other view-related arrays.

- xrt_device: Added new function `xrt_device_get_view_configuration` to get the view configuration for a device. Array size is determined by `XRT_MAX_VIEWS`.
