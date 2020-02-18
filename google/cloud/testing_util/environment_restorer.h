// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_TESTING_UTIL_ENVIRONMENT_RESTORER_H
#define GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_TESTING_UTIL_ENVIRONMENT_RESTORER_H

#include "google/cloud/optional.h"
#include "google/cloud/version.h"
#include <map>
#include <string>

namespace google {
namespace cloud {
inline namespace GOOGLE_CLOUD_CPP_NS {
namespace testing_util {

/**
 * Helper class to restore the value of environment variables.
 */
class EnvironmentRestorer {
 public:
  EnvironmentRestorer() = default;
  ~EnvironmentRestorer();

  // Not copyable.
  EnvironmentRestorer(EnvironmentRestorer const&) = delete;
  EnvironmentRestorer& operator=(EnvironmentRestorer const&) = delete;

  /// Modifications to the environment that will be undone on destruction.
  void SetEnv(std::string const& variable, optional<std::string> value);
  void UnsetEnv(std::string const& variable) { SetEnv(variable, {}); }

 private:
  std::map<std::string, optional<std::string>> undo_;
};

}  // namespace testing_util
}  // namespace GOOGLE_CLOUD_CPP_NS
}  // namespace cloud
}  // namespace google

#endif  // GOOGLE_CLOUD_CPP_GOOGLE_CLOUD_TESTING_UTIL_ENVIRONMENT_RESTORER_H
