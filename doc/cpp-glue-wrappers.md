# C++ glue wrappers {#cpp-glue-wrappers}

<!--
Copyright 2026, NVIDIA CORPORATION.
SPDX-License-Identifier: BSL-1.0
-->

[TOC]

Most of Monado's internal APIs are C interfaces (`@ref xrt_device`,
`@ref xrt_hand_tracker`, and so on). The headers in `src/xrt/auxiliary/glue/`
hold CRTP glue wrappers for some of them. A wrapper lets a C++ class inherit
from a helper base that wires up the C function pointers in its constructor and
recovers the C++ object when C code calls back in.

This page covers how they work, how to use them, and their limitations.

## Overview

| Wrapper | Header | C interface | Recovery helper |
|---------|--------|-------------|-----------------|
| `@ref DeviceBase` | `glue/g_device.hpp` | `@ref xrt_device` | `fromXDev()` |
| `@ref HandTrackerBase` | `glue/g_hand_tracker.hpp` | `@ref xrt_hand_tracker` | `fromXHT()` |
| `@ref FrameServerBase` | `glue/g_frameserver.hpp` | `@ref xrt_fs` | `fromXFS()` |
| `@ref FrameNodeBase` | `glue/g_frame_node.hpp` | `@ref xrt_frame_node` | `fromFrameNode()` |

All wrappers live in namespace `xrt::util`. The callback wrappers use
`g_catch_guard.hpp` to log uncaught C++ exceptions and return
`XRT_ERROR_UNCAUGHT_EXCEPTION` (or nothing, for destroy callbacks).

## Basic usage pattern

Each wrapper is a class template parameterized by the concrete implementation
type `T`, and `T` inherits from it. This is the Curiously Recurring Template
Pattern (CRTP):

```cpp
class MyHandTracker : public xrt::util::HandTrackerBase<MyHandTracker>
{
public:
    MyHandTracker() { /* HandTrackerBase ctor wires C callbacks */ }

    xrt_result_t locate(/* ... */) { /* ... */ }
    xrt_result_t setOutput(/* ... */) { /* ... */ }

    static void destroyHandTracker(struct xrt_hand_tracker *xht)
    {
        delete MyHandTracker::fromXHT(xht);
    }
};
```

Creation and export to C code:

```cpp
auto *tracker = new MyHandTracker(/* ... */);
*out_xht = tracker->getXHT(); // pass this pointer to C callers
```

When C code invokes a callback, the glue receives a pointer to the embedded C
struct (for example `xrt_hand_tracker *`), recovers the glue base subobject
(`HandTrackerBase *`), downcasts that to `T` via `derived()`
(`static_cast<T &>`), and then calls the matching C++ member on `T` (or, for
destruction, a static method; see below).

## How pointer recovery works

### Outbound: C++ to C

`getXHT()`, `getXDev()`, and the other `getX*()` helpers return the address of
the embedded C struct member (`mHandTracker`, `mDevice`, etc.), not `this`.

### Inbound: C callback to C++

The recovery chain is always C struct pointer, then glue base, then `T`, never a
direct cast from the C pointer to `T *`.

Example from `@ref HandTrackerBase`:

```cpp
static T *fromXHT(xrt_hand_tracker *xht) noexcept
{
    return &(reinterpret_cast<HandTrackerBase *>(xht)->derived());
}

const T &derived() noexcept
{
    return static_cast<T &>(*this);
}
```

The `reinterpret_cast` to the glue base works because the embedded C struct is
the first data member of that base. When the base has
[standard layout](https://en.cppreference.com/w/cpp/types/is_standard_layout),
C++ guarantees a pointer to its first member is pointer-interconvertible with a
pointer to the base itself (`[basic.compound]`). This is the familiar
"first member" / `container_of` trick.

The `static_cast` to `T` is then valid because `HandTrackerBase<T>` is a direct,
non-virtual base of `T`, and the object really is a `T` (`[expr.static.cast]`).

### What `T` does not need to be

`T` itself does not need standard layout. It can hold `std::array`, pointers,
non-trivial constructors, and anything else. Only the glue base layout and the
inheritance relationship matter, not the shape of `T`.

## Requirements and limitations

### What you have to get right

For the recovery to be well defined:

- `T` publicly inherits the glue base with its own type as the CRTP argument
  (`HandTrackerBase<T>`, and so on).
- That glue base is a direct, non-virtual base of `T`.
- C callbacks only ever get the pointer returned by `getX*()` on a live `T`.
- The object is constructed and destroyed as a `T` (see destruction below).

### What you do not have to worry about

A few things that look like requirements but are not:

- `T` does not have to be standard layout; only the glue base does, and that is
  what makes the first-member trick legal.
- The glue base does not have to be the first base class of `T`. Recovery goes
  through the glue base subobject, not the start of `T`.
- The glue base has no virtual functions on purpose. Dispatch happens through
  the C function pointers plus CRTP, so there is no vtable.

### Subclassing an existing wrapper user

The template parameter fixes CRTP dispatch to `T`. If you write:

```cpp
class ExtendedTracker : public HandTracker { /* ... */ };
```

pointer recovery still hands you a `HandTracker *` / `HandTracker &`, not an
`ExtendedTracker`. That can still be fine if the methods the glue calls are
virtual, since the overrides on `ExtendedTracker` are then reached through
dynamic dispatch. Destruction is the catch: `HandTracker::destroyHandTracker(xht)`
does `delete HandTracker::fromXHT(xht)`, so unless `HandTracker` has a virtual
destructor, deleting an `ExtendedTracker` this way is undefined behavior.

`HandTracker` today has neither virtual methods nor a virtual destructor. Whether
to inherit from the glue base directly or subclass an existing wrapper user is
a judgement call; see [Method and destroy conventions](#method-and-destroy-conventions).

```cpp
class ExtendedTracker : public xrt::util::HandTrackerBase<ExtendedTracker> { /* ... */ };
```

### Virtual inheritance

Do not use virtual inheritance for the glue base. Base-to-derived recovery
relies on a `static_cast` from the base to `T`, which is ill-formed for a
virtual base, so the wrappers static_assert against it and it will not compile.

### Arbitrary C pointers

The C interface pointer you hand back must be the embedded member of a live `T`.
Passing anything else, such as a copy, a stack temporary, or an unrelated
allocation, is undefined behavior.

## Method and destroy conventions

The instance methods the glue calls on `T` can be plain or virtual, depending on
what you need. Plain methods are the cheaper default; the CRTP base dispatches to
them by name and there is no vtable involved. Virtual methods cost a bit more,
but they let you subclass an existing wrapper user such as `HandTracker` and
reuse its implementation. If you do not need that reuse, inheriting from the glue
base directly is usually simpler; if you do, subclassing the wrapper user can be
worth the cost. It is a judgement call.

Destruction is handled a little differently. The C `destroy` callback calls a
static method on `T` that takes the C interface pointer, recovers the object
with `fromX*()`, and deletes it. Name it after the wrapped interface, such as
`destroyHandTracker` or `destroyDevice`, so it does not clash with other
`destroy` symbols.

## Exception safety

The static callback wrappers wrap their bodies in `try` / `catch` using
`G_CATCH_GUARDS` and `G_CATCH_GUARDS_VOID` from `g_catch_guard.hpp`. An uncaught
C++ exception is logged and turned into `XRT_ERROR_UNCAUGHT_EXCEPTION`, or simply
swallowed for the void destroy callbacks. Never rely on an exception crossing the
C ABI boundary; return an `xrt_result_t` error code for failures you expect.

## Related reading

- `@ref conventions` — general Monado C/C++ API style
- `@ref writing-driver` — implementing `@ref xrt_device` drivers
- `@ref xrt_iface` — internal C interface definitions
- `tests/tests_glue_device.cpp` — unit tests for `@ref DeviceBase`
