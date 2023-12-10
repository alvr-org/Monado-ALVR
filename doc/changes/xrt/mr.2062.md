Introduce two new objects `xrt_system`, `xrt_session`, `xrt_session_event` and
`xrt_session_event_sink`. Along with two new error returns
`XRT_ERROR_IPC_COMPOSITOR_NOT_CREATED` and `XRT_ERROR_COMPOSITOR_NOT_SUPPORTED`.
Also moves the compositor events to the session.
