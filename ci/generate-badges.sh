#!/usr/bin/env bash
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

# Use this script to generate the badges in the README.md file.
if [[ -z "${PROJECT_ROOT+x}" ]]; then
  PROJECT_ROOT="$(cd "$(dirname "$0")/.."; pwd)"
  readonly PROJECT_ROOT
fi

source "${PROJECT_ROOT}/ci/etc/repo-config.sh"

CI_DIR="${PROJECT_ROOT}/ci"
readonly CI_DIR

echo
echo '**Core Builds**'
find "${CI_DIR}/kokoro/macos" \
     "${CI_DIR}/kokoro/docker" \
     "${CI_DIR}/kokoro/windows" \
    \( -name '*-presubmit.cfg' -o -name 'common.cfg' \) -prune \
    -o -name '*.cfg' -print0 |
  while IFS= read -r -d $'\0' file; do
    base="$(basename "${file}" .cfg)"
    dir="$(dirname "${file}")"
    prefix="$(basename "${dir}")"
    echo "[![CI status ${prefix}/${base}][${prefix}/${base}-shield]][${prefix}/${base}-link]"
  done |
  sort

cat <<'_EOF_'
[![Code Coverage Status][codecov-io-badge]][codecov-io-link]
[![Link to Reference Documentation][doxygen-shield]][doxygen-link]

**Install Instructions**
_EOF_

find "${CI_DIR}/kokoro/install" \
    \( -name '*-presubmit.cfg' -o -name 'common.cfg' \) -prune \
    -o -name '*.cfg' -print0 |
  while IFS= read -r -d $'\0' file; do
    base="$(basename "${file}" .cfg)"
    dir="$(dirname "${file}")"
    prefix="$(basename "${dir}")"
    echo "[![CI status ${prefix}/${base}][${prefix}/${base}-shield]][${prefix}/${base}-link]"
  done |
  sort

# We need at least one blank line before the link definitions.
echo
BADGE_FOLDER="https://storage.googleapis.com/cloud-cpp-kokoro-status/${GOOGLE_CLOUD_CPP_REPOSITORY_SHORT}"
find "${CI_DIR}/kokoro/macos" \
     "${CI_DIR}/kokoro/docker" \
     "${CI_DIR}/kokoro/install" \
     "${CI_DIR}/kokoro/windows" \
    \( -name '*-presubmit.cfg' -o -name 'common.cfg' \) -prune \
    -o -name '*.cfg' -print0 |
  while IFS= read -r -d $'\0' file; do
    base="$(basename "${file}" .cfg)"
    dir="$(dirname "${file}")"
    prefix="$(basename "${dir}")"
    echo "[${prefix}/${base}-shield]: ${BADGE_FOLDER}/${prefix}/${base}.svg"
    echo "[${prefix}/${base}-link]: ${BADGE_FOLDER}/${prefix}/${base}-link.html"
  done |
  sort

cat <<_EOF_
[codecov-io-badge]: https://codecov.io/gh/googleapis/${GOOGLE_CLOUD_CPP_REPOSITORY}/branch/master/graph/badge.svg
[codecov-io-link]: https://codecov.io/gh/googleapis/${GOOGLE_CLOUD_CPP_REPOSITORY}
[doxygen-shield]: https://img.shields.io/badge/documentation-master-brightgreen.svg
[doxygen-link]: https://googleapis.github.io/${GOOGLE_CLOUD_CPP_REPOSITORY}/latest/

_EOF_

exit 0
