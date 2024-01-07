main: Let sub-classed targets override compositor extents. The big win here is
that targets no longer writes the `preferred_[width|height]` on the compositor's
settings struct. And this moves us closer to not using `comp_compositor` or
`comp_settings` in the targets which means they can be refactored out of main
and put into util, lettings us reuse them, and even have multiple targets active
at the same time.
