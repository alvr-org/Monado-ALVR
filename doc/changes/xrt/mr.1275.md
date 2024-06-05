---
- mr.1275
- mr.1279
- mr.1283
- mr.1285
- mr.1299
---
Introduce `xrt_system_devices` struct and `xrt_instance_create_system` call. This is a prerequisite for builders, also has the added benefit of moving the logic of which devices has which role solely into the service/instance.
