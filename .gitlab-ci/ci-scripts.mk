# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: 2022-2023 Collabora, Ltd. and the Monado contributors
#
# To generate all the templated files, run this from the root of the repo:
#   make -f .gitlab-ci/ci-scripts.mk

# These also all have their template named the same with a .jinja suffix in the template subdir.
FILES_IN_SUBDIR := \
    .gitlab-ci/distributions \
    .gitlab-ci/reprepro.sh \
    .gitlab-ci/install-android-sdk.sh \

CONFIG_FILE := .gitlab-ci/config.yml
OUTPUTS := .gitlab-ci.yml \
    $(FILES_IN_SUBDIR)

all: $(OUTPUTS)
	chmod +x .gitlab-ci/*.sh
.PHONY: all

clean:
	rm -f $(OUTPUTS)
.PHONY: clean

CI_FAIRY := ci-fairy generate-template --config=$(CONFIG_FILE)

# Because we have includes, and it goes to a different directory, this is special cased
.gitlab-ci.yml: .gitlab-ci/templates/.gitlab-ci.yml.jinja $(CONFIG_FILE)
	$(CI_FAIRY) $< > $@
# Extra deps
.gitlab-ci.yml: .gitlab-ci/templates/include.win_containers.yml
.gitlab-ci.yml: .gitlab-ci/templates/include.ci-containers-variables.yml.jinja
.gitlab-ci.yml: .gitlab-ci/templates/include.ci-containers.yml.jinja
.gitlab-ci.yml: .gitlab-ci/templates/include.functions.jinja

# Everything else is structured alike
$(FILES_IN_SUBDIR): .gitlab-ci/%: .gitlab-ci/templates/%.jinja $(CONFIG_FILE)
	$(CI_FAIRY) $< > $@
