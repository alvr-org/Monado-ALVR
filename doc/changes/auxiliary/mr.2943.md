u/pacing: Compute the fake compositor pacer's next present time in O(1)
instead of O(uptime), which was progressively starving the compositor
over long Android sessions.
