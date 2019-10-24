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

read_into_variable() {
  # Reading a heredoc into a variable with `read` always exits with an error.
  read -r -d '' "${1}" || true;
}

# We need a way to replace "variables" into a chunk of text, but this is
# complicated because the text may (and does) contain shell special characters.
# So we use `sed` to replace @VAR@ by the value of the variable, which is also
# complicated because the text may contain newlines, and sed special characters.
# With enough escaping things work for our purposes.
replace_fragments() {
  local fragment_names=("$@")

  local sed_args=("-e" "s,@GOOGLE_CLOUD_CPP_REPOSITORY@,${GOOGLE_CLOUD_CPP_REPOSITORY},")
  for fragment in "${fragment_names[@]}"; do
    sed_args+=("-e" "s,@${fragment}@,$(/bin/echo -n "${!fragment}" |
        tr '\n' '^' |
        sed -e 's,\\,\\\\,g' -e 's/&/\\&/g' -e 's/,/\\,/g' -e 's/`/\\`/' ),")
  done

  sed "${sed_args[@]}" | tr '^' '\n'
}
