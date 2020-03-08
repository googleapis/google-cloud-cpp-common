// Copyright 2018 Google LLC
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

#include "google/cloud/internal/random.h"
#include <gmock/gmock.h>
#include <future>
#include <vector>

using namespace google::cloud::internal;

TEST(Random, Basic) {
  // This is not a statistical test for PRNG, basically we want to make
  // sure that MakeDefaultPRNG uses different seeds, or at least creates
  // different series:
  auto gen_string = []() {
    auto g = MakeDefaultPRNG();
    return Sample(g, 32, "0123456789abcdefghijklm");
  };
  std::string s0 = gen_string();
  std::string s1 = gen_string();
  EXPECT_NE(s0, s1);
}

// The bug the C++ standard library throws an exception when the bug is
// triggered, without exceptions the test would just crash.
#if GOOGLE_CLOUD_CPP_HAVE_EXCEPTIONS
// @test verify that multiple threads can call
TEST(Random, Threads) {
  auto constexpr kNumWorkers = 64;
  auto constexpr kIterations = 100;
  std::vector<std::future<int>> workers(kNumWorkers);

  std::generate_n(workers.begin(), workers.size(), [&] {
    return std::async(std::launch::async, [&] {
      for (auto i = 0; i != kIterations; ++i) {
        auto g = MakeDefaultPRNG();
        (void)g();
      }
      return kIterations;
    });
  });

  int count = 0;
  for (auto& f : workers) {
    SCOPED_TRACE("testing with worker " + std::to_string(count++));
    int result = 0;
    EXPECT_NO_THROW(result = f.get());
    EXPECT_EQ(result, kIterations);
  }
}
#endif  // GOOGLE_CLOUD_CPP_HAVE_EXCEPTIONS
