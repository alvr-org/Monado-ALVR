---
- mr.873
- mr.1517
- mr.841
- mr.1998
- mr.2001
---
- New compute-shader based rendering backend in the compositor. Supports
  projection, quad, equirect2, cylinder layres. It is not enabled by default. It
  also supports ATW. On some hardware the use of a compute queue improves
  latency when pre-empting other GPU work.
