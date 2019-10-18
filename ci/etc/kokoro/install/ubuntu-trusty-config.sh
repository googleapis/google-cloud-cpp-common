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
# Install the minimal development tools.
# We use the `ubuntu-toolchain-r` PPA to get a modern version of CMake:

# ```bash
RUN apt update && apt install -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test -y
RUN apt update && \
    apt install -y automake cmake3 git gcc g++ libtool make pkg-config \
        tar wget zlib1g-dev
# ```

_EOF_

read -r -d '' INSTALL_BINARY_DEPENDENCIES <<'_EOF_'
_EOF_

read -r -d '' INSTALL_SOURCE_DEPENDENCIES <<'_EOF_'
# #### OpenSSL

# Ubuntu:14.04 ships with a very old version of OpenSSL, this version is not
# supported by gRPC. We need to compile and install OpenSSL-1.0.2 from source.

# ```bash
WORKDIR /var/tmp/build
RUN wget -q https://www.openssl.org/source/openssl-1.0.2n.tar.gz
RUN tar xf openssl-1.0.2n.tar.gz
WORKDIR /var/tmp/build/openssl-1.0.2n
RUN ./config --shared
RUN make -j ${NCPU:-4}
RUN make install
# ```

# Note that by default OpenSSL installs itself in `/usr/local/ssl`. Installing
# on a more conventional location, such as `/usr/local` or `/usr`, can break
# many programs in your system. OpenSSL 1.0.2 is actually incompatible with
# with OpenSSL 1.0.0 which is the version expected by the programs already
# installed by Ubuntu 14.04.

# In any case, as the library installs itself in this non-standard location, we
# also need to configure CMake and other build program to find this version of
# OpenSSL:

# ```bash
ENV OPENSSL_ROOT_DIR=/usr/local/ssl
ENV PKG_CONFIG_PATH=/usr/local/ssl/lib/pkgconfig
# ```

# #### libcurl.

# Because google-cloud-cpp uses both gRPC and curl, we need to compile libcurl
# against the same version of OpenSSL:

# ```bash
WORKDIR /var/tmp/build
RUN wget -q https://curl.haxx.se/download/curl-7.61.0.tar.gz
RUN tar xf curl-7.61.0.tar.gz
WORKDIR /var/tmp/build/curl-7.61.0
RUN ./configure --prefix=/usr/local/curl
RUN make -j ${NCPU:-4}
RUN make install
RUN ldconfig
# ```

# #### crc32c

# There is no Ubuntu-14.04 package for this library. To install it, use:

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

# While Protobuf-2.5 is distributed with Ubuntu-14.04, the Google Cloud Plaform
# proto files require more recent versions (circa 3.4.x). To manually install a
# more recent version use:

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

# #### c-ares

# Recent versions of gRPC require c-ares >= 1.11, while Ubuntu-16.04
# distributes c-ares-1.10. We can manually install a newer version
# of c-ares:

# ```bash
WORKDIR /var/tmp/build
RUN wget -q https://github.com/c-ares/c-ares/archive/cares-1_14_0.tar.gz
RUN tar -xf cares-1_14_0.tar.gz
WORKDIR /var/tmp/build/c-ares-cares-1_14_0
RUN ./buildconf && ./configure && make -j ${NCPU:-4}
RUN make install
# ```

# #### gRPC

# Ubuntu-14.04 does not provide a package for gRPC. Manually install this
# library:

# ```bash
ENV PKG_CONFIG_PATH=/usr/local/ssl/lib/pkgconfig:/usr/local/curl/lib/pkgconfig
WORKDIR /var/tmp/build
RUN wget -q https://github.com/grpc/grpc/archive/v1.19.1.tar.gz
RUN tar -xf v1.19.1.tar.gz
WORKDIR /var/tmp/build/grpc-1.19.1
RUN make -j ${NCPU:-4}
RUN make install
# ```

_EOF_

read -r -d '' ENABLE_USR_LOCAL_FRAGMENT <<'_EOF_'
_EOF_
