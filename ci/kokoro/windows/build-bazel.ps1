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

Write-Host -ForegroundColor Yellow "`n$(Get-Date -Format o) Capture Bazel information for troubleshooting"
bazel version

$common_flags = @()
if (Test-Path env:RUNNING_CI) {
    # Create output directory for Bazel. Bazel creates really long paths,
    # sometimes exceeding the Windows limits. Using a short name for the
    # root of the Bazel output directory works around this problem.
    $bazel_root="C:\b"
    if (-not (Test-Path $bazel_root)) {
        Write-Host -ForegroundColor Yellow "`n$(Get-Date -Format o) Create bazel user root (${bazel_root})"
        New-Item -ItemType Directory -Path $bazel_root | Out-Null
    }
    $common_flags += ("--output_user_root=${bazel_root}")
}
$test_flags = @("--test_output=errors",
                "--verbose_failures=true",
                "--keep_going")
$build_flags = @("--keep_going")

$env:BAZEL_VC="C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC"

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

Write-Host -ForegroundColor Yellow "`n$(Get-Date -Format o) DONE"
