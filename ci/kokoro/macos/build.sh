#!/usr/bin/env bash
#
# Copyright 2018 Google LLC
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

echo "Running build and tests"
cd "$(dirname "$0")/../../.."
readonly PROJECT_ROOT="${PWD}"

echo
echo "================================================================"
echo "================================================================"
echo "Update or Install Bazel."
echo

# macOS does not have sha256sum by default, but `shasum -a 256` does the same
# thing:
function sha256sum() { shasum -a 256 "$@" ; } && export -f sha256sum

"${PROJECT_ROOT}/ci/install-bazel.sh"

readonly BAZEL_BIN="$HOME/bin/bazel"
echo "Using Bazel in ${BAZEL_BIN}"

# The -DGRPC_BAZEL_BUILD is needed because gRPC does not compile on macOS unless
# it is set.
"${BAZEL_BIN}" test \
    --copt=-DGRPC_BAZEL_BUILD \
    --test_output=errors \
    --verbose_failures=true \
    --keep_going \
    -- //google/cloud/...:all

echo
echo "================================================================"
echo "================================================================"
"${BAZEL_BIN}" build \
    --copt=-DGRPC_BAZEL_BUILD \
    --test_output=errors \
    --verbose_failures=true \
    --keep_going \
    -- //google/cloud/...:all
