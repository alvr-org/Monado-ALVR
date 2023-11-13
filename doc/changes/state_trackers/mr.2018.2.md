---
- mr.2018
- mr.2033
- mr.2027
---

OpenXR: Export local_floor if extension enabled and space is supported, since
the extension is compile time we may break the space if the system actually
doesn't support local_floor. Hopefully those cases should be rare.
