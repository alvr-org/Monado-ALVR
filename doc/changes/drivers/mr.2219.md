Set both orientation and position valid flags in the android driver's get_tracked_pose call-back.

hello_xr, unity and possibly other apps check the view pose flags for both position & orientation flags to be valid otherwise they invoke `xrEndFrame` with no layers set causing a constant Gray screen.
