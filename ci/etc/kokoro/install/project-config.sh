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

declare -A ORIGINAL_COPYRIGHT_YEAR=(
  [centos-7]=2018
  [centos-8]=2019
  [debian-buster]=2018
  [debian-stretch]=2018
  [fedora]=2018
  [opensuse-leap]=2019
  [opensuse-tumbleweed]=2018
  [ubuntu-xenial]=2018
  [ubuntu-bionic]=2018
  [ubuntu-focal]=2020
)

BUILD_AND_TEST_PROJECT_FRAGMENT=$(replace_fragments \
      "INSTALL_CPP_CMAKEFILES_FROM_SOURCE" \
      "INSTALL_GOOGLETEST_FROM_SOURCE" <<'_EOF_'
# #### googleapis

# We need a recent version of the Google Cloud Platform proto C++ libraries:

# ```bash
@INSTALL_CPP_CMAKEFILES_FROM_SOURCE@
# ```

# #### googletest

# We need a recent version of GoogleTest to compile the unit and integration
# tests.

# ```bash
@INSTALL_GOOGLETEST_FROM_SOURCE@
# ```

FROM devtools AS install
ARG NCPU=4

# #### Compile and install the main project

# We can now compile, test, and install `@GOOGLE_CLOUD_CPP_REPOSITORY@`.

# ```bash
WORKDIR /home/build/project
COPY . /home/build/project
RUN cmake -H. -Bcmake-out
RUN cmake --build cmake-out -- -j "${NCPU:-4}"
WORKDIR /home/build/project/cmake-out
RUN ctest -LE integration-tests --output-on-failure
RUN cmake --build . --target install
# ```

## [END INSTALL.md]

# Verify that the installed files are actually usable
WORKDIR /home/build/test-install-plain-make
COPY ci/test-install /home/build/test-install-plain-make
COPY google/cloud/samples/common_install_test.cc /home/build/test-install-plain-make
RUN make

WORKDIR /home/build/test-install-cmake
COPY ci/test-install /home/build/test-install-cmake
COPY google/cloud/samples/common_install_test.cc /home/build/test-install-cmake
# Always unset PKG_CONFIG_PATH before building with CMake, this is to ensure
# that CMake does not depend on pkg-config to discover the project.
RUN env -u PKG_CONFIG_PATH cmake -H. -B/i
RUN cmake --build /i -- -j "${NCPU:-4}"
_EOF_
)
