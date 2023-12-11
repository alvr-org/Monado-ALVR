---
- mr.2057
- mr.2072
---

u/builder: Introduce new `u_builder` to make it easier to implement the
interface function `xrt_builder::open_system`. Allowing lots of de-duplication
of code that was exactly the same in most builders.
