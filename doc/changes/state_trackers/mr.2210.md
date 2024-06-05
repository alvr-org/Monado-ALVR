---
- issue.375
---
OpenXR: Fix null-pointer crash bug in `xrGetVisibilityMaskKHR` with in-process builds and replicates the same behaviour as out-of-process builds of falling back to a default implementation.
