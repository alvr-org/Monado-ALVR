OpenXR: Make many more extensions build time options, doesn't change which are
enabled by default. This lets runtimes using Monado control which extensions are
exposed, this needs to be build time options because extensions are listed
before a connection is made to the service.
