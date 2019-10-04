# !/usr/bin/env powershell
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

# Stop on errors. This is similar to `set -e` on Unix shells.
$ErrorActionPreference = "Stop"

# First check the required environment variables.
if (-not (Test-Path env:CONFIG)) {
    throw "Aborting build because the CONFIG environment variable is not set."
}
$CONFIG = $env:CONFIG

# Update or clone the 'vcpkg' package manager, this is a bit overly complicated,
# but it works well on your workstation where you may want to run this script
# multiple times while debugging vcpkg installs.  It also works on AppVeyor
# where we cache the vcpkg installation, but it might be empty on the first
# build.
Write-Host -ForegroundColor Yellow "`n$(Get-Date -Format o) Obtaining vcpkg repository."
Set-Location ..
if (Test-Path vcpkg\.git) {
    Set-Location vcpkg
    git pull
} elseif (Test-Path vcpkg\installed) {
    Move-Item vcpkg vcpkg-tmp
    git clone https://github.com/Microsoft/vcpkg
    Move-Item vcpkg-tmp\installed vcpkg
    Set-Location vcpkg
} else {
    git clone https://github.com/Microsoft/vcpkg
    Set-Location vcpkg
}
if ($LastExitCode) {
    throw "vcpkg git setup failed with exit code $LastExitCode"
}

# If BUILD_CACHE is set (which typically is on Kokoro builds), try
# to download and extract the build cache.
if (Test-Path env:BUILD_CACHE) {
    Write-Host -ForegroundColor Yellow "`n$(Get-Date -Format o) Downloading build cache."
    gsutil cp $BUILD_CACHE vcpkg-installed.zip
    if ($LastExitCode) {
        # Ignore errors, caching failures should not break the build.
        Write-Host "gsutil download failed with exit code $LastExitCode"
    }
    Write-Host -ForegroundColor Yellow "`n$(Get-Date -Format o) Extracting build cache."
    7z x vcpkg-installed.zip -aoa
    if ($LastExitCode) {
        # Ignore errors, caching failures should not break the build.
        Write-Host "extracting build cache failed with exit code $LastExitCode"
    }

}

Write-Host -ForegroundColor Yellow "`n$(Get-Date -Format o) Bootstrap vcpkg."
powershell -exec bypass scripts\bootstrap.ps1
if ($LastExitCode) {
    throw "vcpkg bootstrap failed with exit code $LastExitCode"
}

# Only compile the release version of the packages we need, for Debug builds
# this trick does not work, so we need to compile all versions (yuck).
if ($CONFIG -eq "Release") {
    Add-Content triplets\x64-windows-static.cmake "set(VCPKG_BUILD_TYPE release)"
}

# Integrate installed packages into the build environment.
.\vcpkg.exe integrate install
if ($LastExitCode) {
    throw "vcpkg integrate failed with exit code $LastExitCode"
}

# Remove old versions of the packages.
Write-Host -ForegroundColor Yellow "`n$(Get-Date -Format o) Cleanup old vcpkg package versions."
.\vcpkg.exe remove --outdated --recurse
if ($LastExitCode) {
    throw "vcpkg remove --outdated failed with exit code $LastExitCode"
}

Write-Host -ForegroundColor Yellow "`n$(Get-Date -Format o) Building vcpkg package versions."
$packages = @("zlib", "openssl",
              "protobuf", "c-ares",
              "grpc", "gtest",
              "googleapis")
foreach ($pkg in $packages) {
    .\vcpkg.exe install "${pkg}:${env:VCPKG_TRIPLET}"
    if ($LastExitCode) {
        throw "vcpkg install $pkg failed with exit code $LastExitCode"
    }
}

Write-Host -ForegroundColor Yellow "`n$(Get-Date -Format o) vcpkg list"
.\vcpkg.exe list

if (Test-Path env:BUILD_CACHE) {
    Write-Host -ForegroundColor Yellow "`n$(Get-Date -Format o) Create cache zip file."
    7z a vcpkg-installed.zip installed\
    if ($LastExitCode) {
        # Ignore errors, caching failures should not break the build.
        Write-Host "zip build cache failed with exit code $LastExitCode"
    }

    Write-Host -ForegroundColor Yellow "`n$(Get-Date -Format o) Upload cache zip file."
    gsutil cp vcpkg-installed.zip $BUILD_CACHE
    if ($LastExitCode) {
        # Ignore errors, caching failures should not break the build.
        Write-Host "gsutil upload failed with exit code $LastExitCode"
    }
}
