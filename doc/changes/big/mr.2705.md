a/util: allow devices to be reassigned as different roles

Allows devices to be reassigned as different roles through environment
variables. Currently, devices are assigned to roles based on the order that they
appear, which causes randomness in which device gets assigned as what.
This also allows for multiple of the same device to be plugged in at the same
time, and allow the user to pick which one gets used.

This can be useful when testing multiple HMDs, or when using controllers for FBT
and wanting to avoid the randomness of which one gets assigned first.
