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

#include "google/cloud/testing_util/environment_restorer.h"
#include "google/cloud/internal/getenv.h"
#include "google/cloud/internal/setenv.h"
#include <gtest/gtest.h>

namespace google {
namespace cloud {
inline namespace GOOGLE_CLOUD_CPP_NS {
namespace testing_util {
namespace {

constexpr auto kVarName = "ENVIRONMENT_RESTORER_TEST";

TEST(EnvironmentRestorer, SetOverSet) {
  EnvironmentRestorer env_outer;
  env_outer.SetEnv(kVarName, "foo");
  EXPECT_EQ(*internal::GetEnv(kVarName), "foo");
  {
    EnvironmentRestorer env_inner;
    env_inner.SetEnv(kVarName, "bar");
    EXPECT_EQ(*internal::GetEnv(kVarName), "bar");
  }
  EXPECT_EQ(*internal::GetEnv(kVarName), "foo");
}

TEST(EnvironmentRestorer, SetOverUnset) {
  EnvironmentRestorer env_outer;
  env_outer.UnsetEnv(kVarName);
  EXPECT_FALSE(internal::GetEnv(kVarName).has_value());
  {
    EnvironmentRestorer env_inner;
    env_inner.SetEnv(kVarName, "bar");
    EXPECT_EQ(*internal::GetEnv(kVarName), "bar");
  }
  EXPECT_FALSE(internal::GetEnv(kVarName).has_value());
}

TEST(EnvironmentRestorer, UnsetOverSet) {
  EnvironmentRestorer env_outer;
  env_outer.SetEnv(kVarName, "foo");
  EXPECT_EQ(*internal::GetEnv(kVarName), "foo");
  {
    EnvironmentRestorer env_inner;
    env_inner.UnsetEnv(kVarName);
    EXPECT_FALSE(internal::GetEnv(kVarName).has_value());
  }
  EXPECT_EQ(*internal::GetEnv(kVarName), "foo");
}

}  // namespace
}  // namespace testing_util
}  // namespace GOOGLE_CLOUD_CPP_NS
}  // namespace cloud
}  // namespace google
