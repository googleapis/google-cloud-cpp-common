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

#include "google/cloud/testing_util/scoped_environment.h"
#include "google/cloud/internal/getenv.h"
#include "google/cloud/internal/setenv.h"

namespace google {
namespace cloud {
inline namespace GOOGLE_CLOUD_CPP_NS {
namespace testing_util {

ScopedEnvironment::~ScopedEnvironment() {
  for (auto& undo : undo_) {
    internal::SetEnv(undo.first.c_str(), std::move(undo.second));
  }
}

void ScopedEnvironment::SetEnv(std::string const& variable,
                               optional<std::string> value) {
  auto it = undo_.lower_bound(variable);
  if (it == undo_.end() || it->first != variable) {
    undo_.insert(it, {variable, internal::GetEnv(variable.c_str())});
  }
  internal::SetEnv(variable.c_str(), value);
}

}  // namespace testing_util
}  // namespace GOOGLE_CLOUD_CPP_NS
}  // namespace cloud
}  // namespace google
