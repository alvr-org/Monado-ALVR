# OpenXR async functions (XrFutureEXT)  {#async}

<!--
Copyright 2025, Collabora, Ltd. and the Monado contributors
SPDX-License-Identifier: BSL-1.0
-->

This document describes the extra steps required when implementing OpenXR extensions that expose asynchronous functions returning `XrFutureEXT` in Monado. The overall process is the same as implementing normal extensions (see [implementing-extension](./implementing-extension.md)), but there are a few additional points to keep in mind — mostly around future result types, lifetime management, and IPC support.

---

## Future result types

1. If the future's data result is a struct, the data type must be an **xrt** data-type (typically defined in `xrt_defines.h`).
2. Register that xrt data-type in `xrt_future_value` (in `xrt_future_value.h`) by adding a new entry to `XRT_FUTURE_VALUE_TYPES[_WITH]`.

---

## Server-side / driver overview

OpenXR async functions come in pairs of `xrDoWorkAsync[Suffix]` and `xrDoWorkComplete[Suffix]`. When adding these functions to the server-side/driver (and for IPC) you typically **do not** need to implement the `Complete` function — for simple use-cases where you only need to obtain results, the `Complete` implementation is not required. For more complex scenarios you may still need to implement the `Complete` function.

---

## Server-side / driver — implementing async callbacks

1. Add a callback to the device (for example `xrt_device::create_foo_object_async`).

   - Name the callback data member with a `_async` suffix.
   - The last parameter **must** be an output parameter of type `struct xrt_future **`.

2. In the implementation that is hooked up to this callback:

   - Create/derive a specific future instance (for example via `u_future_create`).
   - `xrt_future` objects are reference-counted for shared access and thread-safe destruction.
   - If the asynchronous work runs on a different thread than the callback caller, or if you need a local reference to the future, increment and decrement the reference count when crossing thread/object boundaries using `xrt_future_reference`.

### Example callback (driver-side)

```cpp
//! simulated driver
static xrt_result_t
simulated_create_foo_object_async(struct xrt_device *xdev, struct xrt_future **out_future)
{
	struct simulated_hmd *hmd = simulated_hmd(xdev);
	if (hmd == nullptr || out_future == nullptr) {
		return XRT_ERROR_ALLOCATION;
	}

	struct xrt_future *xft = u_future_create();
	if (xft == nullptr) {
		return XRT_ERROR_ALLOCATION;
	}

	struct xrt_future *th_xft = NULL;
	xrt_future_reference(&xft, xft);
	assert(th_xft != nullptr);

	std::thread t([th_xft]() mutable {
		using namespace std::chrono_literals;

		bool is_cancel_requested = false;
		for (uint32_t x = 0; x < 100; ++x) {
			if (xrt_future_is_cancel_requested(th_xft, &is_cancel_requested) != XRT_SUCCESS ||
				is_cancel_requested) {
				U_LOG_I("cancelling work...");
				break;
			}
			U_LOG_I("doing work...");
			std::this_thread::sleep_for(250ms);
		}

		struct xrt_future_result rest;
		if (!is_cancel_requested) {
			struct xrt_foo foo_bar = {
				.foo = 65.f,
				.bar = 19,
			};
			rest = XRT_FUTURE_RESULT(foo_bar, XRT_SUCCESS);
		}
		xrt_future_complete(th_xft, &rest);
		xrt_future_reference(&th_xft, nullptr);
		U_LOG_I("work finished...");
	});
	t.detach();

	*out_future = xft;

	return XRT_SUCCESS;
}
```

---

## Server-side IPC

When adding async support to IPC, keep these conventions in mind:

1. In `proto.json`:

   - `xrt_future**` output parameters are represented by integer IDs.
   - Use `uint32_t` for future IDs. Example:

   ```json
   "device_create_foo_object_async": {
	"in": [
		{"name": "id", "type": "uint32_t"}
	],
	"out": [
		{"name": "out_future_id", "type": "uint32_t"}
	]
   }
   ```

2. In the server-side handler (`ipc_server_handler.c`):

   - Use `get_new_future_id` to obtain a new future ID.
   - Add the newly created `xrt_future` returned from the callback into the client's future list (`struct ipc_client_state::xfts`) using that ID.

### Example server handler

```c
xrt_result_t
ipc_handle_device_create_foo_object_async(volatile struct ipc_client_state *ics, uint32_t id, uint32_t *out_future_id)
{
	struct xrt_device *xdev = NULL;
	GET_XDEV_OR_RETURN(ics, id, xdev);

	uint32_t new_future_id;
	xrt_result_t xret = get_new_future_id(ics, &new_future_id);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	struct xrt_future *xft = NULL;
	xret = xrt_device_create_foo_object_async(xdev, &xft);
	if (xret != XRT_SUCCESS) {
		return xret;
	}

	assert(xft != NULL);
	assert(new_future_id < IPC_MAX_CLIENT_FUTURES);
	ics->xfts[new_future_id] = xft;

	*out_future_id = new_future_id;
	return XRT_SUCCESS;
}
```

---

## Client-side IPC

On the client side, create an IPC-backed future using the ID returned by the server; call `ipc_client_future_create` to construct a client-side `xrt_future` wrapper for that ID.

### Example client handler

```c
static xrt_result_t
ipc_client_xdev_create_foo_object_async(struct xrt_device *xdev, struct xrt_future **out_future)
{
	if (xdev == NULL || out_future == NULL) {
		return XRT_ERROR_INVALID_ARGUMENT;
	}
	struct ipc_client_xdev *icx = ipc_client_xdev(xdev);

	uint32_t future_id;
	xrt_result_t r = ipc_call_device_create_foo_object_async(icx->ipc_c, icx->device_id, &future_id);
	if (r != XRT_SUCCESS) {
		IPC_ERROR(icx->ipc_c, "Error sending create_foo_object_async!");
		return r;
	}

	struct xrt_future *new_future = ipc_client_future_create(icx->ipc_c, future_id);
	if (new_future == NULL) {
		return XRT_ERROR_ALLOCATION;
	}

	*out_future = new_future;
	return XRT_SUCCESS;
}
```

---

## State tracker (OpenXR layer)

Implementing the OpenXR async function hooks requires implementing both functions in the async pair. In those functions:

- Use `oxr_future_ext` and the operations declared in `oxr_object.h`. This provides a close 1:1 mapping between OpenXR futures/async functions and Monado's internal future types.
- Note: OpenXR describes `XrFutureEXT` as a new primitive that is neither a handle nor an atom. In Monado, however, `oxr_future_ext` is still represented like other `oxr_*` types (i.e., it behaves like a handle/atom in the internal mapping).
- The lifetime of `oxr_future_ext` is tied to the parent handle passed into `oxr_future_create`. This means:
  - You do not need to store the future by value in parent `oxr_` types.
  - You *must* ensure the future's parent handle is set correctly. Do not parent the future to `oxr_instance` or `oxr_session` unless that truly represents the future's intended lifetime scope.

---

## Example / reference

A full end-to-end example of adding an OpenXR extension with async functions is available [here](https://gitlab.freedesktop.org/korejan/monado/-/commits/korejan/ext_future_example)
