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

"""Load dependencies needed to compile and test the google-cloud-cpp library."""

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive", "http_file")

def google_cloud_cpp_common_deps():
    """Loads dependencies need to compile the google-cloud-cpp-common library.

    Application developers can call this function from their WORKSPACE file
    to obtain all the necessary dependencies for google-cloud-cpp-common,
    including gRPC and its dependencies. This function only loads dependencies
    that have not been previously loaded, allowing application developers to
    override the version of the dependencies they want to use.
    """

    # Load rules_cc, used by googletest
    if "rules_cc" not in native.existing_rules():
        http_archive(
            name = "rules_cc",
            strip_prefix = "rules_cc-master",
            urls = [
                "https://github.com/bazelbuild/rules_cc/archive/a508235df92e71d537fcbae0c7c952ea6957a912.tar.gz",
            ],
            sha256 = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        )

    # Load a version of googletest that we know works.
    if "com_google_googletest" not in native.existing_rules():
        http_archive(
            name = "com_google_googletest",
            strip_prefix = "googletest-release-1.10.0",
            urls = [
                "https://github.com/google/googletest/archive/release-1.10.0.tar.gz",
            ],
            sha256 = "9dc9157a9a1551ec7a7e43daea9a694a0bb5fb8bec81235d8a1e6ef64c716dcb",
        )

    # Load the googleapis dependency.
    if "com_google_googleapis" not in native.existing_rules():
        http_archive(
            name = "com_google_googleapis",
            urls = [
                "https://github.com/googleapis/googleapis/archive/a8ee1416f4c588f2ab92da72e7c1f588c784d3e6.tar.gz",
            ],
            strip_prefix = "googleapis-a8ee1416f4c588f2ab92da72e7c1f588c784d3e6",
            sha256 = "6b8a9b2bcb4476e9a5a9872869996f0d639c8d5df76dd8a893e79201f211b1cf",
            build_file = "@com_github_googleapis_google_cloud_cpp_common//bazel:googleapis.BUILD",
        )

    # Load gRPC and its dependencies, using a similar pattern to this function.
    # This implictly loads "com_google_protobuf", which we use.
    if "com_github_grpc_grpc" not in native.existing_rules():
        http_archive(
            name = "com_github_grpc_grpc",
            strip_prefix = "grpc-1.23.0",
            urls = [
                "https://github.com/grpc/grpc/archive/v1.23.0.tar.gz",
                "https://mirror.bazel.build/github.com/grpc/grpc/archive/v1.23.0.tar.gz",
            ],
            sha256 = "f56ced18740895b943418fa29575a65cc2396ccfa3159fa40d318ef5f59471f9",
        )

    # We need libcurl for the Google Cloud Storage client.
    if "com_github_curl_curl" not in native.existing_rules():
        http_archive(
            name = "com_github_curl_curl",
            urls = [
                "https://mirror.bazel.build/curl.haxx.se/download/curl-7.60.0.tar.gz",
                "https://curl.haxx.se/download/curl-7.60.0.tar.gz",
            ],
            strip_prefix = "curl-7.60.0",
            sha256 = "e9c37986337743f37fd14fe8737f246e97aec94b39d1b71e8a5973f72a9fc4f5",
            build_file = "@com_github_googleapis_google_cloud_cpp//bazel:curl.BUILD",
        )

    # We need the nlohmann_json library
    if "com_github_nlohmann_json_single_header" not in native.existing_rules():
        http_file(
            name = "com_github_nlohmann_json_single_header",
            urls = [
                "https://github.com/nlohmann/json/releases/download/v3.4.0/json.hpp",
            ],
            sha256 = "63da6d1f22b2a7bb9e4ff7d6b255cf691a161ff49532dcc45d398a53e295835f",
        )

    # Load google/crc32c, a library to efficiently compute CRC32C checksums.
    if "com_github_google_crc32c" not in native.existing_rules():
        http_archive(
            name = "com_github_google_crc32c",
            strip_prefix = "crc32c-1.0.6",
            urls = [
                "https://github.com/google/crc32c/archive/1.0.6.tar.gz",
            ],
            sha256 = "6b3b1d861bb8307658b2407bc7a4c59e566855ef5368a60b35c893551e4788e9",
            build_file = "@com_github_googleapis_google_cloud_cpp//bazel:crc32c.BUILD",
        )

    # We use the cc_proto_library() rule from @com_google_protobuf, which
    # assumes that grpc_cpp_plugin and grpc_lib are in the //external: module
    native.bind(
        name = "grpc_cpp_plugin",
        actual = "@com_github_grpc_grpc//:grpc_cpp_plugin",
    )

    native.bind(
        name = "grpc_lib",
        actual = "@com_github_grpc_grpc//:grpc++",
    )
