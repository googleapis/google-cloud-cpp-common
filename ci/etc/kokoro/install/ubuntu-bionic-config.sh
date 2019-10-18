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
# Install the minimal development tools, libcurl, OpenSSL and libc-ares:

# ```bash
RUN apt update && \
    apt install -y build-essential cmake git gcc g++ cmake \
        libc-ares-dev libc-ares2 libcurl4-openssl-dev libssl-dev make \
        pkg-config tar wget zlib1g-dev
# ```

_EOF_

read -r -d '' INSTALL_BINARY_DEPENDENCIES <<'_EOF_'
_EOF_

INSTALL_SOURCE_DEPENDENCIES=$(
  cat <<'_EOF_'
# #### crc32c

# There is no CentOS package for this library. To install it use:

# ```bash
_EOF_
  echo "${INSTALL_CRC32C_FROM_SOURCE}"

  cat <<'_EOF_'
# ```

# #### Protobuf

# Likewise, manually install protobuf:

# ```bash
_EOF_
  echo "${INSTALL_PROTOBUF_FROM_SOURCE}"

  cat <<'_EOF_'
# ```

# #### gRPC

# Can be manually installed using:

# ```bash
_EOF_
  echo "${INSTALL_GRPC_FROM_SOURCE}"
  echo '# ```'
)

read -r -d '' ENABLE_USR_LOCAL_FRAGMENT <<'_EOF_'
_EOF_
