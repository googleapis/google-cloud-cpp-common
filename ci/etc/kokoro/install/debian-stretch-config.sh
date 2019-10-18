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
# First install the development tools and libcurl.
# On Debian 9, libcurl links against openssl-1.0.2, and one must link
# against the same version or risk an inconsistent configuration of the library.
# This is especially important for multi-threaded applications, as openssl-1.0.2
# requires explicitly setting locking callbacks. Therefore, to use libcurl one
# must link against openssl-1.0.2. To do so, we need to install libssl1.0-dev.
# Note that this removes libssl-dev if you have it installed already, and would
# prevent you from compiling against openssl-1.1.0.

# ```bash
RUN apt update && \
    apt install -y build-essential cmake git gcc g++ cmake \
        libc-ares-dev libc-ares2 libcurl4-openssl-dev libssl1.0-dev make \
        pkg-config tar wget zlib1g-dev
# ```

_EOF_

read -r -d '' INSTALL_BINARY_DEPENDENCIES <<'_EOF_'
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

# #### Protobuf

# While protobuf-3.0 is distributed with Ubuntu, the Google Cloud Plaform proto
# files require more recent versions (circa 3.4.x). To manually install a more
# recent version use:

# ```bash
WORKDIR /var/tmp/build
RUN wget -q https://github.com/google/protobuf/archive/v3.6.1.tar.gz
RUN tar -xf v3.6.1.tar.gz
WORKDIR /var/tmp/build/protobuf-3.6.1/cmake
RUN cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=yes \
        -Dprotobuf_BUILD_TESTS=OFF \
        -H. -Bcmake-out
RUN cmake --build cmake-out --target install -- -j ${NCPU:-4}
RUN ldconfig
# ```

# #### gRPC

# Likewise, Ubuntu has packages for grpc-1.3.x, but this version is too old for
# the Google Cloud Platform APIs:

# ```bash
WORKDIR /var/tmp/build
RUN wget -q https://github.com/grpc/grpc/archive/v1.19.1.tar.gz
RUN tar -xf v1.19.1.tar.gz
WORKDIR /var/tmp/build/grpc-1.19.1
RUN make -j ${NCPU:-4}
RUN make install
RUN ldconfig
# ```

_EOF_

read -r -d '' ENABLE_USR_LOCAL_FRAGMENT <<'_EOF_'
ENV PKG_CONFIG_PATH=/usr/local/lib64/pkgconfig
_EOF_
