// Copyright 2021, Collabora Ltd.
// Author: Nova King <technobaboo@proton.me>
// SPDX-License-Identifier: BSL-1.0

// RGB to HSV conversion. Returns vec3(h, s, v) with h in [0, 1].
vec3 rgb_to_hsv(vec3 c)
{
	float cmax = max(c.r, max(c.g, c.b));
	float cmin = min(c.r, min(c.g, c.b));
	float delta = cmax - cmin;

	float h = 0.0;
	if (delta > 0.0) {
		if (cmax == c.r) {
			h = mod((c.g - c.b) / delta, 6.0) / 6.0;
		} else if (cmax == c.g) {
			h = ((c.b - c.r) / delta + 2.0) / 6.0;
		} else {
			h = ((c.r - c.g) / delta + 4.0) / 6.0;
		}
	}

	float s = (cmax > 0.0) ? (delta / cmax) : 0.0;
	return vec3(h, s, cmax);
}

// HSV to RGB conversion. Expects vec3(h, s, v) with h in [0, 1].
vec3 hsv_to_rgb(vec3 hsv)
{
	float c = hsv.z * hsv.y;
	float h6 = hsv.x * 6.0;
	float x = c * (1.0 - abs(mod(h6, 2.0) - 1.0));
	float m = hsv.z - c;

	vec3 rgb;
	if (h6 < 1.0) rgb = vec3(c, x, 0.0);
	else if (h6 < 2.0) rgb = vec3(x, c, 0.0);
	else if (h6 < 3.0) rgb = vec3(0.0, c, x);
	else if (h6 < 4.0) rgb = vec3(0.0, x, c);
	else if (h6 < 5.0) rgb = vec3(x, 0.0, c);
	else rgb = vec3(c, 0.0, x);

	return rgb + m;
}

// Compute how far a value is outside the [lo, hi] range, normalized.
// Returns 0 when inside, 1 when at or beyond the range width away.
float range_distance(float val, float lo, float hi)
{
	if (lo >= hi) return 1.0; // Degenerate range, no match
	float below = max(lo - val, 0.0);
	float above = max(val - hi, 0.0);
	return max(below, above) / (hi - lo);
}

// Same as range_distance but handles hue wrapping in [0, 1].
float hue_distance(float h, float lo, float hi)
{
	if (lo < hi) {
		// Normal range
		return range_distance(h, lo, hi);
	}
	// Wrapped range (e.g. min=0.9, max=0.1 means red spanning 0)
	// Inside if h >= lo OR h <= hi
	if (h >= lo || h <= hi) return 0.0;
	float dist_to_lo = min(abs(h - lo), 1.0 - abs(h - lo));
	float range = 1.0 - lo + hi;
	return dist_to_lo / max(range, 0.001);
}
