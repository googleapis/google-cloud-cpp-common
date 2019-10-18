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
RUN apt update && \
    apt install -y build-essential cmake git gcc g++ cmake \
        libssl-dev make pkg-config tar wget zlib1g-dev
# ```
_EOF_

read -r -d '' INSTALL_BINARY_DEPENDENCIES <<'_EOF_'
# Debian 10 includes versions of gRPC and Protobuf that support the
# Google Cloud Platform proto files. We simply install these pre-built versions:

# ```bash
RUN apt update && \
    apt install -y libcurl4-openssl-dev libgrpc++-dev libprotobuf-dev \
        protobuf-compiler protobuf-compiler-grpc
# ```
_EOF_

read -r -d '' INSTALL_SOURCE_DEPENDENCIES <<'_EOF_'

# #### crc32c

# There is no Debian package for this library. To install it use:

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
