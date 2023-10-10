render: Refactor gfx descriptor pool, descriptor layout creation function,
ubo upload ad descriptor updating function to be shareable. The common pattern
is one UBO and one source image, so make it possible to share these.
