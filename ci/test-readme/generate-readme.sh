#!/usr/bin/env bash
#
# Copyright 2019 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set -eu

readonly BINDIR=$(dirname "$0")

badge() {
  local -r distro="$1"
  cat <<_EOF_

[![Kokoro readme ${distro} status][kokoro-readme-${distro}-shield]][kokoro-readme-${distro}-link]

[kokoro-readme-${distro}-shield]: https://storage.googleapis.com/cloud-cpp-kokoro-status/common/readme/${distro}.svg
[kokoro-readme-${distro}-link]: https://storage.googleapis.com/cloud-cpp-kokoro-status/common/readme/${distro}-link.html
_EOF_
}

readonly DOCKERFILES_DIR="${BINDIR}/../kokoro/readme"

echo "<!-- Start of automatically generated content by ci/$(basename "$0") -->"

echo "### CentOS (8)"
badge centos-8
"${BINDIR}/extract-readme.sh" "${DOCKERFILES_DIR}/Dockerfile.centos-8"

echo "### CentOS (7)"
badge centos
"${BINDIR}/extract-readme.sh" "${DOCKERFILES_DIR}/Dockerfile.centos"

echo "### Debian (10 - Buster)"
badge debian-buster
"${BINDIR}/extract-readme.sh" "${DOCKERFILES_DIR}/Dockerfile.debian-buster"

echo "### Debian (9 - Stretch)"
badge debian-stretch
"${BINDIR}/extract-readme.sh" "${DOCKERFILES_DIR}/Dockerfile.debian-stretch"

echo "### Fedora (30)"
badge fedora
"${BINDIR}/extract-readme.sh" "${DOCKERFILES_DIR}/Dockerfile.fedora"

echo "### openSUSE (Tumbleweed)"
badge opensuse-tumbleweed
"${BINDIR}/extract-readme.sh" "${DOCKERFILES_DIR}/Dockerfile.opensuse-tumbleweed"

echo "### openSUSE (Leap)"
badge opensuse-leap
"${BINDIR}/extract-readme.sh" "${DOCKERFILES_DIR}/Dockerfile.opensuse-leap"

echo "### Ubuntu (18.04 - Bionic Beaver)"
badge ubuntu-bionic
"${BINDIR}/extract-readme.sh" "${DOCKERFILES_DIR}/Dockerfile.ubuntu-bionic"

echo "### Ubuntu (16.04 - Xenial Xerus)"
badge ubuntu-xenial
"${BINDIR}/extract-readme.sh" "${DOCKERFILES_DIR}/Dockerfile.ubuntu-xenial"

echo "### Ubuntu (14.04 - Trusty Tahr)"
badge ubuntu-trusty
"${BINDIR}/extract-readme.sh" "${DOCKERFILES_DIR}/Dockerfile.ubuntu-trusty"

echo "<!-- End of automatically generated content -->"
