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

read -r -d '' INSTALL_COMMON_SOURCE_DEPENDENCIES <<'_EOF_'

# #### googleapis

# We need a recent version of the Google Cloud Platform proto C++ libraries:

# ```bash
WORKDIR /var/tmp/build
RUN wget -q https://github.com/googleapis/cpp-cmakefiles/archive/v0.1.5.tar.gz
RUN tar -xf v0.1.5.tar.gz
WORKDIR /var/tmp/build/cpp-cmakefiles-0.1.5
RUN cmake \
    -DBUILD_SHARED_LIBS=YES \
    -H. -Bcmake-out
RUN cmake --build cmake-out --target install -- -j ${NCPU:-4}
RUN ldconfig
# ```

# #### googletest

# We need a recent version of GoogleTest to compile the unit and integration
# tests.

# ```bash
WORKDIR /var/tmp/build
RUN wget -q https://github.com/google/googletest/archive/release-1.10.0.tar.gz
RUN tar -xf release-1.10.0.tar.gz
WORKDIR /var/tmp/build/googletest-release-1.10.0
RUN cmake \
      -DCMAKE_BUILD_TYPE="Release" \
      -DBUILD_SHARED_LIBS=yes \
      -H. -Bcmake-out
RUN cmake --build cmake-out --target install -- -j ${NCPU:-4}
RUN ldconfig
# ```
_EOF_


read -r -d '' TEST_INSTALLED_PROJECT_FRAGMENT <<'_EOF_'

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
RUN env -u PKG_CONFIG_PATH cmake -H. -Bcmake-out
RUN cmake --build cmake-out -- -j ${NCPU:-4}
_EOF_
