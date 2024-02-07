
- u/u_config_json: Added new parameter `uint32_t *out_view_count` to the function `u_config_json_get_remote_settings` to provide the ability to retrieve the view count from the remote settings.

- u/device: Added new function `u_device_setup_one_eye`.

- u/u_distortion: Modified the function `u_distortion_cardboard_calculate` to accept a new parameter `struct xrt_device *xdev` for retrieving the `view_count` from the device. This `view_count` is then used for parameter settings, enhancing the functionality and flexibility of the distortion calculation.
