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

if [[ $# -ne 1 ]]; then
  echo "Usage: $(basename "$0") <destination-ci-directory>"
  exit 1
fi

if [[ -z "${PROJECT_ROOT+x}" ]]; then
  PROJECT_ROOT="$(cd "$(dirname "$0")/../.."; pwd)"
  readonly PROJECT_ROOT
fi

DESTINATION_ROOT="$1"
readonly DESTINATION_ROOT

source "${DESTINATION_ROOT}/ci/etc/repo-config.sh"

BUILD_NAMES=(
  centos-7
  centos-8
  debian-buster
  debian-stretch
  fedora
  opensuse-leap
  opensuse-tumbleweed
  ubuntu-trusty
  ubuntu-xenial
  ubuntu-bionic
)
readonly BUILD_NAMES

generate_dockerfile() {
  local -r build="$1"

  if [[ "${build}" == "fedora" ]]; then
    local distro="fedora"
    local distro_version="30"
  elif [[ "${build}" == "opensuse-leap" ]]; then
    local distro="opensuse/leap"
    local distro_version=latest
  elif [[ "${build}" == "opensuse-tumbleweed" ]]; then
    local distro="opensuse/tumbleweed"
    local distro_version=latest
  else
    split=( ${build//-/ } )
    if [[ "${#split[@]}" != "2" ]]; then
      echo "Invalid build id [${build}], expected <distro>-<distro_version>"
      exit 1
    fi
    local distro="${split[0]}"
    local distro_version="${split[1]}"
  fi
  readonly distro
  readonly distro_version

  set +e
  source "${DESTINATION_ROOT}/ci/etc/kokoro/install/project-config.sh"
  source "${DESTINATION_ROOT}/ci/etc/kokoro/install/${build}-config.sh"
  set -e

  target="${DESTINATION_ROOT}/ci/kokoro/install/Dockerfile.${build}"


  echo "Generating ${target} ... ${distro} ${distro_version} ${build}"

  cat >"${target}" <<_EOF_
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

ARG DISTRO_VERSION=${distro_version}
FROM ${distro}:\${DISTRO_VERSION} AS devtools
ARG NCPU=4

# This is an automatically generated file, do not modify it directly, see
#   https://github.com/googleapis/google-cloud-cpp-common/ci/templates

## [START INSTALL.md]

_EOF_

  echo "${INSTALL_DEVTOOLS_FRAGMENT}" >>"${target}"
  echo "${INSTALL_BINARY_DEPENDENCIES}" >>"${target}"
  echo "${INSTALL_SOURCE_DEPENDENCIES}" >>"${target}"
  echo "${INSTALL_COMMON_SOURCE_DEPENDENCIES}" >>"${target}"

  # We cannot use simple shell expansion in the next fragment because the
  # backticks (needed for formatting) are interpreted as "launch shell".
  sed -e "s/@GOOGLE_CLOUD_CPP_REPOSITORY@/${GOOGLE_CLOUD_CPP_REPOSITORY}/" \
      >>${target} <<'_EOF_'

FROM devtools AS install

# #### Compile and install the main project

# We can now compile and install `@GOOGLE_CLOUD_CPP_REPOSITORY@`.

# ```bash
WORKDIR /home/build/project
COPY . /home/build/project
RUN cmake -H. -B/o
RUN cmake --build /o -- -j ${NCPU:-4}
RUN cmake --build /o --target install
WORKDIR /o
RUN ctest --output-on-failure
# ```

## [END INSTALL.md]

_EOF_

  echo "${ENABLE_USR_LOCAL_FRAGMENT}" >>"${target}"
  echo "${TEST_INSTALLED_PROJECT_FRAGMENT}" >>"${target}"
}

# Remove all files, any files we want to preserve will be created again.
git -C "${DESTINATION_ROOT}" rm -fr "ci/kokoro/install" || true

mkdir -p "${DESTINATION_ROOT}/ci/kokoro/install"

cp "${PROJECT_ROOT}/ci/templates/kokoro/install/build.sh" \
   "${DESTINATION_ROOT}/ci/kokoro/install/build.sh"

sed -e "s/@GOOGLE_CLOUD_CPP_REPOSITORY@/${GOOGLE_CLOUD_CPP_REPOSITORY}/" \
  "${PROJECT_ROOT}/ci/templates/kokoro/install/common.cfg.in" \
  >"${DESTINATION_ROOT}/ci/kokoro/install/common.cfg"

for build in "${BUILD_NAMES[@]}"; do
  touch "${DESTINATION_ROOT}/ci/kokoro/install/${build}.cfg"
  touch "${DESTINATION_ROOT}/ci/kokoro/install/${build}-presubmit.cfg"
  generate_dockerfile "${build}"
done

git -C "${DESTINATION_ROOT}" add "ci/kokoro/install"
