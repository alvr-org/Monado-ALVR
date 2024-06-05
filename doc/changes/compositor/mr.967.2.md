util: Refactor swapchain and fence code to be more independent of compositor
and put into own library. Joined by a `comp_base` helper that implements
a lot of the more boilerplate compositor code.
