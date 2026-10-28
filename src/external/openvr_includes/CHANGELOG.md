# OpenVR headers

Pulled from upstream [OpenVR respository](https://github.com/ValveSoftware/openvr/).

## Current version

* Synced on 2020-06-10 `OpenVR SDK 1.12.5`
* Synced on 2022-07-27 `OpenVR SDK 1.16.8`
* Synced on 2026-04-17 `OpenVR SDK 2.15.6`

## Manual headers

* `IVRClientCore_003.h` - Pulled from [here](https://github.com/ValveSoftware/openvr/blob/0924064316de3effbcd1acf1e309182a2deb1c05/src/ivrclientcore.h).
* `openvr_old_types.h` - Any types used in older versions of OpenVR structures that are no longer used.

## Generated headers

* `openvr_interfaces_unified.h` - Contains every interface version in a single large header.
* `openvr_forward_macros.h` - Contains macros to forward implementations from older interface versions -> newer versions when signatures match.

## Generation instructions

1. `$ python3 export_versioned_headers.py /path/to/openvr/repository -o /tmp/openvr_versioned/`
2. `$ python3 generate_unified_interfaces.py -i /tmp/openvr_versioned/ -o /tmp/openvr_versioned/openvr_interfaces_unified.h`
3. Copy `openvr_interfaces_unified.h` from `/tmp/openvr_versioned/`.
