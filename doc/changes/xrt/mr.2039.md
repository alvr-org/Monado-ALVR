Add `XRT_ERROR_DEVICE_FUNCTION_NOT_IMPLEMENTED` error message, used to signal
when a function that isn't implemented is called. It is not meant to query the
availability of the function or feature, only a error condition on bad code.
