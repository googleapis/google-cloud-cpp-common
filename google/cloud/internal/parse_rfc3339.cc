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

#include "google/cloud/internal/parse_rfc3339.h"
#include "google/cloud/internal/throw_delegate.h"
#include <array>
#include <cctype>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace {
[[noreturn]] void ReportError(std::string const& timestamp, char const* msg) {
  std::ostringstream os;
  os << "Error parsing RFC 3339 timestamp: " << msg
     << " Valid format is YYYY-MM-DD[Tt]HH:MM:SS[.s+](Z|[+-]HH:MM), got="
     << timestamp;
  google::cloud::internal::ThrowInvalidArgument(os.str());
}

bool IsLeapYear(int year) {
  // NOLINTNEXTLINE(readability-magic-numbers)
  return (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
}

auto constexpr kMonthsInYear = 12;
auto constexpr kHoursInDay = 24;
auto constexpr kMinutesInHour =
    std::chrono::seconds(std::chrono::minutes(1)).count();
auto constexpr kSecondsInMinute =
    std::chrono::minutes(std::chrono::hours(1)).count();

std::chrono::system_clock::time_point ParseDateTime(
    char const*& buffer, std::string const& timestamp) {
  // Use std::mktime to compute the number of seconds because RFC 3339 requires
  // working with civil time, including the annoying leap seconds, and mktime
  // does.
  int year, month, day;  // NOLINT(readability-isolate-declaration)
  char date_time_separator;
  int hours, minutes, seconds;  // NOLINT(readability-isolate-declaration)

  int pos;
  auto count =
      std::sscanf(buffer, "%4d-%2d-%2d%c%2d:%2d:%2d%n", &year, &month, &day,
                  &date_time_separator, &hours, &minutes, &seconds, &pos);
  // All the fields up to this point have fixed width, so total width must be:
  auto constexpr kExpectedWidth = 19;
  auto constexpr kExpectedFields = 7;
  if (count != kExpectedFields || pos != kExpectedWidth) {
    ReportError(timestamp,
                "Invalid format for RFC 3339 timestamp detected while parsing"
                " the base date and time portion.");
  }
  if (date_time_separator != 'T' && date_time_separator != 't') {
    ReportError(timestamp, "Invalid date-time separator, expected 'T' or 't'.");
  }

  // Double braces are needed to workaround a clang-3.8 bug.
  std::array<int, kMonthsInYear> constexpr kMaxDaysInMonth{{
      31,  // January
      29,  // February (non-leap years checked below)
      31,  // March
      30,  // April
      31,  // May
      30,  // June
      31,  // July
      31,  // August
      30,  // September
      31,  // October
      30,  // November
      31,  // December
  }};
  auto constexpr kMkTimeBaseYear = 1900;
  if (month < 1 || month > kMonthsInYear) {
    ReportError(timestamp, "Out of range month.");
  }
  if (day < 1 || day > kMaxDaysInMonth[month - 1]) {
    ReportError(timestamp, "Out of range day for given month.");
  }
  if (2 == month && day > kMaxDaysInMonth[1] - 1 && !IsLeapYear(year)) {
    ReportError(timestamp, "Out of range day for given month.");
  }
  if (hours < 0 || hours >= kHoursInDay) {
    ReportError(timestamp, "Out of range hour.");
  }
  if (minutes < 0 || minutes >= kMinutesInHour) {
    ReportError(timestamp, "Out of range minute.");
  }
  // RFC-3339 points out that the seconds field can only assume value '60' for
  // leap seconds, so theoretically, we should validate that (furthermore, we
  // should valid that `seconds` is smaller than 59 for negative leap seconds).
  // This would require loading a table, and adds too much complexity for little
  // value.
  if (seconds < 0 || seconds > kSecondsInMinute) {
    ReportError(timestamp, "Out of range second.");
  }
  // Advance the pointer for all the characters read.
  buffer += pos;

  std::tm tm{};
  tm.tm_year = year - kMkTimeBaseYear;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hours;
  tm.tm_min = minutes;
  tm.tm_sec = seconds;
  return std::chrono::system_clock::from_time_t(std::mktime(&tm));
}

std::chrono::system_clock::duration ParseFractionalSeconds(
    char const*& buffer, std::string const& timestamp) {
  if (buffer[0] != '.') {
    return std::chrono::system_clock::duration(0);
  }
  ++buffer;

  long fractional_seconds;  // NOLINT(google-runtime-int)
  int pos;
  auto count = std::sscanf(buffer, "%9ld%n", &fractional_seconds, &pos);
  if (count != 1) {
    ReportError(timestamp, "Invalid fractional seconds component.");
  }
  auto constexpr kMaxNanosecondDigits = 9;
  auto constexpr kNanosecondsBase = 10;
  // Normalize the fractional seconds to nanoseconds.
  for (int digits = pos; digits < kMaxNanosecondDigits; ++digits) {
    fractional_seconds *= kNanosecondsBase;
  }
  // Skip any other digits. This loses precision for sub-nanosecond timestamps,
  // but we do not consider this a problem for Internet timestamps.
  buffer += pos;
  while (std::isdigit(buffer[0]) != 0) {
    ++buffer;
  }
  return std::chrono::duration_cast<std::chrono::system_clock::duration>(
      std::chrono::nanoseconds(fractional_seconds));
}

std::chrono::seconds ParseOffset(char const*& buffer,
                                 std::string const& timestamp) {
  if (buffer[0] == '+' || buffer[0] == '-') {
    bool positive = (buffer[0] == '+');
    ++buffer;
    // Parse the HH:MM offset.
    int hours, minutes, pos;  // NOLINT(readability-isolate-declaration)
    auto count = std::sscanf(buffer, "%2d:%2d%n", &hours, &minutes, &pos);
    auto constexpr kExpectedOffsetWidth = 5;
    auto constexpr kExpectedOffsetFields = 2;
    if (count != kExpectedOffsetFields || pos != kExpectedOffsetWidth) {
      ReportError(timestamp, "Invalid timezone offset, expected [+-]HH:MM.");
    }
    if (hours < 0 || hours >= kHoursInDay) {
      ReportError(timestamp, "Out of range offset hour.");
    }
    if (minutes < 0 || minutes >= kMinutesInHour) {
      ReportError(timestamp, "Out of range offset minute.");
    }
    buffer += pos;
    using std::chrono::duration_cast;
    if (positive) {
      return duration_cast<std::chrono::seconds>(std::chrono::hours(hours) +
                                                 std::chrono::minutes(minutes));
    }
    return duration_cast<std::chrono::seconds>(-std::chrono::hours(hours) -
                                               std::chrono::minutes(minutes));
  }
  if (buffer[0] != 'Z' && buffer[0] != 'z') {
    ReportError(timestamp, "Invalid timezone offset, expected 'Z' or 'z'.");
  }
  ++buffer;
  return std::chrono::seconds(0);
}
}  // anonymous namespace

namespace google {
namespace cloud {
inline namespace GOOGLE_CLOUD_CPP_NS {
namespace internal {
std::chrono::system_clock::time_point ParseRfc3339(
    std::string const& timestamp) {
  // TODO(#530) - dynamically change the timezone offset.
  // Because this computation is a bit expensive, assume the timezone offset
  // does not change during the lifetime of the program.  This function takes
  // the current time, converts to UTC and then convert back to a time_t.  The
  // difference is the offset.
  static std::chrono::seconds const kLocalTimeOffset = []() {
    auto now = std::time(nullptr);
    std::tm lcl;
// The standard C++ function to convert time_t to a struct tm is not thread
// safe (it holds global storage), use some OS specific stuff here:
#if _WIN32
    gmtime_s(&lcl, &now);
#else
    gmtime_r(&now, &lcl);
#endif  // _WIN32
    return std::chrono::seconds(mktime(&lcl) - now);
  }();

  char const* buffer = timestamp.c_str();
  auto time_point = ParseDateTime(buffer, timestamp);
  auto fractional_seconds = ParseFractionalSeconds(buffer, timestamp);
  std::chrono::seconds offset = ParseOffset(buffer, timestamp);

  if (buffer[0] != '\0') {
    ReportError(timestamp, "Additional text after RFC 3339 date.");
  }

  time_point += fractional_seconds;
  time_point -= offset;
  time_point -= kLocalTimeOffset;
  return time_point;
}

}  // namespace internal
}  // namespace GOOGLE_CLOUD_CPP_NS
}  // namespace cloud
}  // namespace google
