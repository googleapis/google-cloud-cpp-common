# !/usr/bin/env powershell
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

# Stop on errors. This is similar to `set -e` on Unix shells.
$ErrorActionPreference = "Stop"

$common_flags = @()
if (Test-Path env:RUNNING_CI) {
    # In the Kokoro builds we need to pass this flag.
    $common_flags += ("--output_user_root=C:\b")
}
$test_flags = @("--test_output=errors",
                "--verbose_failures=true",
                "--keep_going")
$build_flags = @("--keep_going")

Write-Host -ForegroundColor Yellow "`n$(Get-Date -Format o) Compiling and running unit tests"
bazel $common_flags test $test_flags -- //google/cloud/...:all
if ($LastExitCode) {
    throw "bazel test failed with exit code $LastExitCode"
}

Write-Host -ForegroundColor Yellow "`n$(Get-Date -Format o) Running extra programs"
bazel $common_flags build $build_flags -- //google/cloud/...:all
if ($LastExitCode) {
    throw "bazel test failed with exit code $LastExitCode"
}
