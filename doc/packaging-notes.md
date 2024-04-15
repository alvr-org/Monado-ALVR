# Packaging Notes {#packaging-notes}

<!--
Copyright 2023-2024, Collabora, Ltd. and the Monado contributors
SPDX-License-Identifier: BSL-1.0
-->

## The SONAME and library location question

_AKA, "Why does `libopenxr_monado.so` not have a versioned SONAME? Can I move it
to a subdirectory of the library path?"_

To understand this, it helps to keep in mind how OpenXR works from an
application point of view. Applications are intended to be runtime-independent
and portable, using the user's current runtime wherever they are executed. The
[OpenXR Loader][] is the library that applications link against to achieve this:
it provides a layer of indirection. It also allows for the use of API layers
with runtimes, and handles runtime finding. Follow that link for more
information from the design documents.

The TL;DR version is this: `libopenxr_monado` not having a versioned SONAME yet
being installed in the default library path is **not an error or oversight**,
but a carefully-reasoned technical decision. Do not patch this or change it in
distribution packages without understanding what design choices and features you
break by doing so.

[OpenXR Loader]: https://registry.khronos.org/OpenXR/specs/1.1/loader.html

### My package linter is complaining that `libopenxr_monado.so` does not have a versioned SONAME

Monado (as well as OpenXR API layers) are like Vulkan drivers (technically ICDs)
and API layers, in that they are only used behind a loader doing runtime dynamic
linking/late loading. A versioned `SONAME` makes no real sense for them, but
sometimes you do want them in the default dynamic library load path.

You never link against the OpenXR runtime interface of Monado directly, it's
loaded dynamically at runtime by the OpenXR loader after reading configurations
and manifest files. The OpenXR loader _does_ have a versioned SONAME on Linux,
where it might be system installed. This is why it makes no sense to have a
SONAME for the runtime SO: the compatibility of that SO is defined by the
manifest that is installed and the OpenXR spec itself.

At some point we might give up and set a versioned SONAME for
`libopenxr_monado`, but it would not improve anything about the software other
than its compliance with external rules that do not consider how the software is
designed or used. Furthermore, it would likely lead to increased
misunderstandings in how OpenXR and Monado are intended to be used by
developers. The only public interface of that `libopenxr_monado.so` is strictly
specified by the [OpenXR specification][] in general, with its exported symbols
specified by the [Runtime Interface Negotiation][] section.

[OpenXR specification]: https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html
[Runtime Interface Negotiation]: https://registry.khronos.org/OpenXR/specs/1.1/html/xrspec.html#api-initialization-runtime-interface-negotiation

Note: `libmonado`, which _does_ have a public API as a management interface for
software serving as a Monado front-end or dashboard, _does_ have a versioned
SONAME.

### My package linter is complaining that without a versioned SONAME, `libopenxr_monado.so` should move to a private subdirectory of `/usr/lib/...`

Why can we not move that library off of the main dynamic loader path? It is true
that OpenXR API layers can live in a subdirectory of the dynamic library path,
because the loader will try to load layers from all the manifests and ignore the
ones whose library it cannot load, resolving multi-arch issues by trying all
architectures. However, until a change to the OpenXR loader in OpenXR SDK 1.0.29
(from late 2023), the only way to get multi-arch support with an OpenXR
_runtime_ on Linux was to:

- Place the binary in the correct library directory for the architecture
- Put only the library name in the runtime manifest (rather than a relative or
  absolute path) - turn off both `XRT_OPENXR_INSTALL_ABSOLUTE_RUNTIME_PATH` and
  `XRT_OPENXR_INSTALL_ABSOLUTE_RUNTIME_PATH` for this to happen.

When this is done, the loader uses the normal library search path to find the
runtime, which gives you the right architecture for the application.
Essentially, it lets the loader use the same argument to `dlopen` regardless of
architecture, and get the right library. This allows, e.g. a 32-bit application
to use the 32-bit version of `libopenxr_monado`, typically to talk to the 64-bit
`monado-service` instance.

At some point it would be nice to generate the architecture-specific runtime
manifests for Monado. However, it makes selecting runtimes system-wide using
`alternatives` or similar more complex, as you would need to update multiple
symlinks.
