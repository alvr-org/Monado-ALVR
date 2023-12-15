Pass down broadcast `xrt_session_event_sink` pointer to prober and builder when
creating system, this is used to broadcast events to all sessions. Such as
reference space change pending event.
