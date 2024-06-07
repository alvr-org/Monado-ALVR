# How to make a release {#how-to-release}

<!--
Copyright 2021, Collabora, Ltd. and the Monado contributors
SPDX-License-Identifier: BSL-1.0
-->

These instructions assumes that the version you are making is `24.0.0`.

## Generate changelog

Run proclamation in the `doc/changes`.

```sh
proclamation build 24.0.0
```

Commit changes.

```sh
git commit -m "doc: Update CHANGELOG" doc/CHANGELOG.md doc/changes
```

## Update versions

Edit the files below to update the version number embedded in them.

* `CMakeLists.txt`
* `vcpkg.json`

See previous commits for exact places.

```sh
git commit -m "monado: Update version" CMakeLists.txt vcpkg.json
```

## Tag the code

Do the tagging from git, do **not** do it from GitLab.
Also, make sure to prefix the version with `v` so that e.g. `24.0.0` becomes `v24.0.0`.
The `-s` flag signs the tag.

```sh
git tag v24.0.0 -m "v24.0.0" -a -s
```

## Do GitLab release

The GitLab UI has a friendly interface, follow the guide there.
