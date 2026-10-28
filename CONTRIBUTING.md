# Contribution Requirements and Guidelines

<!--
Copyright 2018-2026 Collabora, Ltd.

SPDX-License-Identifier: CC-BY-4.0
-->

The following section of this file labeled as "Policy", as well as
the Code of Conduct, available at
<https://www.freedesktop.org/wiki/CodeOfConduct/>, are **MANDATORY**, and
apply in their entirety to anyone involved in the project. See the README for
associated Code of Conduct contacts.

## Policy: Contribution Provenance and Developer Certificate of Origin

Every commit in a Merge Request **MUST** include a `Signed-off-by:` tag in the
commit message. By adding this tag, you agree to the **Developer Certificate of
Origin (DCO) v1.1** found at <https://developercertificate.org/> or in the
source tree as DCO-1.1. This certifies, among other things, that you have the
right to submit the code under the project's license. Before submitting code,
please read the text of the DCO itself for complete details.

Note that only a human can legally certify the DCO. If you are not the original,
sole author of a patch, you must gather `Signed-off-by:` tags from all
contributors to a patch. One of the `Signed-off-by:` tags must match the Git
author metadata for each commit.

Submissions without a valid sign-off (or sign-offs) will be rejected.
---

## Guidelines

Unlike the preceding mandatory policies, there are plenty of valid reasons why
someone might not be able to follow all of the guidelines in this section, and
that's OK, especially for new contributors or those new to open source entirely.
Let us know and we'll figure out a way to help you get involved successfully.

### Large Scale Contributions

For large scale contributions, such as those involving extensive scope, numerous commits, cross-cutting impact, or changes to architecture and APIs, we recommend a structured but collaborative approach to ensure a smooth review process:

- **Align on Design Early**: To minimize back-and-forth during MR reviews, we kindly ask that you open an 'RFC' GitLab Work Item (Issue) or a Draft Merge Request early in your process. Discussing your intended approach beforehand ensures you are aligned with Monado reviewers before investing heavily in writing code. You can begin implementation once at least one core maintainer gives explicit approval, for example, a "thumbs-up" or explicit comment on the thread.

- **Phase Your Work**: Whenever possible, please try to break down large contributions into smaller, phased Merge Requests with focused commits, rather than submitting a single massive update.

- **Keep Discussions Public**: To help build a shared history and context for the whole community, try to keep design and review conversations on public GitLab Issues and Merge Requests.

### Pull/Merge Requests

- If you're considering starting work on a large change that you'd like to contribute,
  it is recommended to first open an issue before you start,
  to begin a discussion and help smooth the acceptance of your contribution.

- If you are able, please make sure to run clang-format
  (ideally version 11 or newer) before each commit,
  so that you only commit things that are cleanly styled.
  Consistent, machine-performed formatting improves readability and makes it easier for others to contribute.
  It also makes it easier to review changes.
  If you can't run clang-format, mention this fact in your request and we'd be happy to help,
  either in a single "Clean up formatting." commit on top of your work,
  or by "re-writing history" (with your permission and leaving your commit authorship intact),
  revising each commit to apply formatting.

  - Android Java and Kotlin code can be formatted using "Spotless" by running `./gradlew spotlessApply`.

- Avoid including whitespace or other formatting changes to unrelated code when committing.
  The `git add -p` command or the "stage selected lines/hunks" feature of various Git GUIs are
  great ways of making sure you only stage and commit the changes that you mean to.
  Relatedly, `git commit -v` (if you commit from the command line) can be a great help
  in making sure you aren't committing things you don't mean to,
  by showing the diff you're committing in your commit message editor.
  (This can even be set system-wide in `git config --global commit.verbose true`
  if you find it as life-changing as many others have - thanks
  [emilyst](https://twitter.com/emilyst/status/1039205453010362368).)

- If you can, before submitting a pull/merge request, try building with clang-tidy enabled,
  and if you touched any code that has or should have documentation,
  build and check the documentation and see if it looks OK.

- We work to keep the code free of warnings -
  please help by making sure your changes build cleanly (and pass all tests).
  When on compilers that take warning flags like gcc and clang do,
  the build system automatically turns on quite a few of them.
  If you can't figure out a warning, mention it in the request so we can figure it out together.

- We use [Proclamation](https://gitlab.com/proclamation/proclamation) to generate release notes.
  After creating a merge request for a substantial change, please create one or more changelog
  fragments on the branch describing the changes.  See
  [doc/changes/README.md](doc/changes/README.md) for more info.

### Issues

Constructive issues are a valued form of contribution.
Please try to include any relevant information
(whether it is a request for improvement or a bug report).
We'll try to respond promptly,
but there is no guarantee or warranty (as noted in the license),
absent any externally-arranged consulting or support contract.

Since this is a runtime/implementation of an API used by other applications,
bug reports should include:

- details about your build environment
  - architecture
  - compiler
  - compiler version
  - build flags (defines, configuration/feature flags)
- associated application code
  - for logic/execution errors, a new (failing) test case is ideal,
    otherwise a description of expected and actual behavior
  - if you cannot disclose your code, or even if you can,
    an "artificial", minimally-sized example can be very valuable.
