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

read -r -d '' INSTALL_DEVTOOLS_FRAGMENT <<'_EOF_'
# Install the minimal development tools:

# ```bash
RUN dnf makecache && \
    dnf install -y cmake gcc-c++ git make openssl-devel pkgconfig \
        zlib-devel
# ```
_EOF_

read -r -d '' INSTALL_BINARY_DEPENDENCIES <<'_EOF_'
# Fedora includes packages for gRPC, libcurl, and OpenSSL that are recent enough
# for the project. Install these packages and additional development tools to
# compile the dependencies:

# ```bash
RUN dnf makecache && \
    dnf install -y grpc-devel grpc-plugins \
        libcurl-devel protobuf-compiler tar wget zlib-devel
# ```
_EOF_

read -r -d '' INSTALL_SOURCE_DEPENDENCIES <<'_EOF_'

# #### crc32c

# There is no CentOS package for this library. To install it use:

# ```bash
WORKDIR /var/tmp/build
RUN wget -q https://github.com/google/crc32c/archive/1.0.6.tar.gz
RUN tar -xf 1.0.6.tar.gz
WORKDIR /var/tmp/build/crc32c-1.0.6
RUN cmake \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=yes \
      -DCRC32C_BUILD_TESTS=OFF \
      -DCRC32C_BUILD_BENCHMARKS=OFF \
      -DCRC32C_USE_GLOG=OFF \
      -H. -Bcmake-out/crc32c
RUN cmake --build cmake-out/crc32c --target install -- -j ${NCPU:-4}
RUN ldconfig
# ```

_EOF_

read -r -d '' ENABLE_USR_LOCAL_FRAGMENT <<'_EOF_'
ENV PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig
_EOF_
