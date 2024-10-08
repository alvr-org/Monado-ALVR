#!/usr/bin/env bash
# SPDX-License-Identifier: CC0-1.0
# SPDX-FileCopyrightText: 2018-2024 Collabora, Ltd. and the Monado contributors

###############################################
#           GENERATED - DO NOT EDIT           #
# see .gitlab-ci/reprepro.sh.template instead #
###############################################


set -euo pipefail

# Convince gnupg to work properly in CI
echo "Import and cache the GPG key"
mkdir -p ~/.gnupg && chmod 700 ~/.gnupg
cp .gitlab-ci/gpg.conf .gitlab-ci/gpg-agent.conf ~/.gnupg
echo RELOADAGENT | gpg-connect-agent
gpg --batch --no-tty --yes --pinentry-mode loopback --passphrase "${MONADO_GPG_PASSPHRASE}" --import "${MONADO_GPG_SECRET_KEY}"

echo "Prepare reprepro config"
mkdir -p repo/conf
# Substitute in the GPG fingerprint into the repository config.
# This file is itself generated with ci-fairy.
cat .gitlab-ci/distributions | envsubst  > repo/conf/distributions

echo "reprepro config file:"
echo "---------------------"
cat repo/conf/distributions
echo "---------------------"

# For each distro, sign the changes file and add it to the repo.

# bookworm
if [ -f "incoming/bookworm.distro" ]; then
    VERSION=$(cat incoming/bookworm.distro)
    echo "Signing and processing bookworm: ${VERSION}"
    debsign -k "${MONADO_GPG_FINGERPRINT}" -p "gpg --batch --no-tty --yes --pinentry-mode loopback --passphrase ${MONADO_GPG_PASSPHRASE}" "incoming/monado_${VERSION}_amd64.changes"
    reprepro -V --ignore=wrongdistribution -b repo include bookworm "incoming/monado_${VERSION}_amd64.changes"
else
    echo "Skipping bookworm - no artifact found"
fi

# bullseye
if [ -f "incoming/bullseye.distro" ]; then
    VERSION=$(cat incoming/bullseye.distro)
    echo "Signing and processing bullseye: ${VERSION}"
    debsign -k "${MONADO_GPG_FINGERPRINT}" -p "gpg --batch --no-tty --yes --pinentry-mode loopback --passphrase ${MONADO_GPG_PASSPHRASE}" "incoming/monado_${VERSION}_amd64.changes"
    reprepro -V --ignore=wrongdistribution -b repo include bullseye "incoming/monado_${VERSION}_amd64.changes"
else
    echo "Skipping bullseye - no artifact found"
fi

# jammy
if [ -f "incoming/jammy.distro" ]; then
    VERSION=$(cat incoming/jammy.distro)
    echo "Signing and processing jammy: ${VERSION}"
    debsign -k "${MONADO_GPG_FINGERPRINT}" -p "gpg --batch --no-tty --yes --pinentry-mode loopback --passphrase ${MONADO_GPG_PASSPHRASE}" "incoming/monado_${VERSION}_amd64.changes"
    reprepro -V --ignore=wrongdistribution -b repo include jammy "incoming/monado_${VERSION}_amd64.changes"
else
    echo "Skipping jammy - no artifact found"
fi

# noble
if [ -f "incoming/noble.distro" ]; then
    VERSION=$(cat incoming/noble.distro)
    echo "Signing and processing noble: ${VERSION}"
    debsign -k "${MONADO_GPG_FINGERPRINT}" -p "gpg --batch --no-tty --yes --pinentry-mode loopback --passphrase ${MONADO_GPG_PASSPHRASE}" "incoming/monado_${VERSION}_amd64.changes"
    reprepro -V --ignore=wrongdistribution -b repo include noble "incoming/monado_${VERSION}_amd64.changes"
else
    echo "Skipping noble - no artifact found"
fi
