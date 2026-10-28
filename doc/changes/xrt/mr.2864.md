Add `xrt_hand_tracker` object, this moves the logic of selecting xdev for
XrHandTracker objects to the service side of the IPC layer. Also letting
external runtimes select different behavior for the hand-tracker without
changing the OpenXR code.
