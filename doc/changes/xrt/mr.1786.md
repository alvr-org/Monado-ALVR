- Add `xrt_compositor::get_display_refresh_rate` so that the application can
  get the current display refresh rate.
- Add `xrt_compositor::request_display_refresh_rate` so that the application can
  trigger a display refresh rate change.
- Add `xrt_compositor_event_display_refresh_rate_change` for compositor can
  notify applications that the display refresh rate has been changed.
