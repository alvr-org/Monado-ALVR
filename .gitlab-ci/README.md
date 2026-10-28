# CI and Generated Stuff Readme

<!--
# Copyright 2018-2025, Collabora, Ltd. and the Monado contributors
#
# SPDX-License-Identifier: CC0-1.0
-->

We use the FreeDesktop
["CI Templates"](https://freedesktop.pages.freedesktop.org/ci-templates) to
maintain build containers using code in this repo, storing the images in GitLab
Registry. Our CI files (and some other files) are auto-generated from Jinja
templates and `config.yml`, using
[ci-fairy](https://freedesktop.pages.freedesktop.org/ci-templates/ci-fairy.html).
You can install it with:

<!-- do not break the following line, it is used in CI setup too, to make sure it works -->
```sh
pipx install git+https://gitlab.freedesktop.org/freedesktop/ci-templates@fb9d50ccb3cbbb4c6dc5f9ef53a0ad3cb0d8a177
```

On Windows you will also need to have GNU make and busybox installed, such as with:

```pwsh
scoop install make busybox
```

To re-generate files, from the root directory, run:

```sh
make -f .gitlab-ci/ci-scripts.mk
```

If you really want to force rebuilding, you can build the clean target first:

```sh
make -f .gitlab-ci/ci-scripts.mk clean all
```
