// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "google/cloud/internal/random.h"
#include <algorithm>
#include <limits>

namespace google {
namespace cloud {
inline namespace GOOGLE_CLOUD_CPP_NS {
namespace internal {
std::vector<unsigned int> FetchEntropy(std::size_t desired_bits) {
  // We use the default C++ random device to generate entropy.  The quality of
  // this source of entropy is implementation-defined. The version in
  // Linux with libstdc++ (the most common library on Linux) it is based on
  // either `/dev/urandom`, or (if available) the RDRAND, or the RDSEED CPU
  // instruction.
  //     http://en.cppreference.com/w/cpp/numeric/random/random_device/random_device
  //
  // On Linux with libc++ it is also based on `/dev/urandom`, but it is not
  // known if it uses the RDRAND CPU instruction (though `/dev/urandom` does).
  //
  // On Windows the documentation says that the numbers are not deterministic,
  // and cryptographically secure, but no further details are available:
  //     https://docs.microsoft.com/en-us/cpp/standard-library/random-device-class
  //
  // One would want to simply pass this object to the constructor for the
  // generator, but the C++11 approach is annoying, see this critique for
  // the details:
  //     http://www.pcg-random.org/posts/simple-portable-cpp-seed-entropy.html

#if defined(__GLIBCXX__) && __GLIBCXX__ >= 20200128
  // Workaround for a libstd++ bug:
  //     https://gcc.gnu.org/bugzilla/show_bug.cgi?id=94087
  // we cannot simply use `rdrand` everywhere because this is library and
  // version specific, i.e., other standard C++ libraries do not support
  // `rdrand`, and even older versions of libstdc++ do not support `rdrand`.
  std::random_device rd("rdrand");
#else
  std::random_device rd;
#endif  // defined(__GLIBCXX__) && __GLIBCXX__ >= 20200128

  auto constexpr kWordSize = std::numeric_limits<unsigned int>::digits;
  auto const n = desired_bits / kWordSize +
                 static_cast<int>(desired_bits % kWordSize != 0);
  std::vector<unsigned int> entropy(n);
  std::generate(entropy.begin(), entropy.end(), [&rd]() { return rd(); });
  return entropy;
}

std::string Sample(DefaultPRNG& gen, int n, std::string const& population) {
  std::uniform_int_distribution<std::size_t> rd(0, population.size() - 1);

  std::string result(std::size_t(n), '0');
  std::generate(result.begin(), result.end(),
                [&rd, &gen, &population]() { return population[rd(gen)]; });
  return result;
}

}  // namespace internal
}  // namespace GOOGLE_CLOUD_CPP_NS
}  // namespace cloud
}  // namespace google
