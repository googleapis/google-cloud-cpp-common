# Changelog

## v0.22.x - TBD

## v0.21.x - 2020-03

* fix: install google_cloud_cpp_testing_grpc headers (#185)

## v0.20.x - 2020-02

* feat: add new support for testing with environment variables (#180)
* feat: logging can be enabled via env var (#181)
* refactor: copy IsProtoEqual from spanner (#179)

## v0.19.x - 2020-02

**BREAKING CHANGES:**
* refactor!: remove grpc_utils namespace; left aliases for backward compat (#158)
  **POTENTIALLY BREAKING CHANGE:** Refactor the types and functions from
  `google::cloud::grpc_utils` to `google::cloud::`. The old header files and
  types should continue to work, as we kept aliases for them, but there is some
  risk we missed something. The library name (the physical .a and/or .so file)
  is not changed, the target names for CMake are not changed. For Bazel, the
  old targets continue to work, but you might want to move to newer targets
  that do not expose the backwards compatibility headers.

**Other Changes:**
* feat: cancel futures returned by .then() (#166)
* feat: support cancellation for long running operations (#160)
* refactor: copy PaginationRange from spanner (#168)
* refactor: copy ConnectionOptions from spanner (#165)
* refactor: Copy `BackgroundThreads` from `google::cloud::spanner`
* refactor: copy TracingOptions from spanner (#161)

## v0.18.x - 2020-02

**BREAKING CHANGES:**
* feat!: change the result type for timers to StatusOr (#134)
  `CompletionQueue::MakeRelativeTimer` and `CompletionQueue::MakeDeadlineTimer()`
  now return a `future<StatusOr<std::chrono::system_clock::time_point>>`.
  This change is needed because now these operations may fail if the completion
  queue is shutdown before the timer is set.
* chore: remove `CompletionQueue` parameter to `AsyncGrpcOperation::Notify` (#136)

**Other Changes:**
* fix: google/cloud/grpc_utils/examples does not exist (#149)
* feat: gracefully fail adding to the `CompletionQueue` after `Shutdown` (#138)
* fix: remove the redundant conditions in the `Run()` loop (#131)
* fix: crashes with non-empty CompletionQueue shutdown (#127)
* chore: move release notes to CHANGELOG.md (#128)

## v0.17.x - 2019-12

* Cleanup and documentation changes only

## v0.16.x - 2019-10

* fix: make `google::cloud::internal::apply` work for `std::tuple<>&`

## v0.15.x - 2019-10

* chore: upgrade to gRPC-1.24.3 (#75)

## v0.14.x - 2019-10

* feat: implement std::apply (#59)

## v0.13.x - 2019-10

* feat: implement install components (#33)

## v0.12.x - 2019-10

* bug: fix runtime install directory (#3063)

## v0.11.x - 2019-09

* feat: Use macros for compiler id and version (#2937)
* fix: Fix bazel build on windows. (#2940)
* chore: Keep release tags in master branch. (#2963)
* cleanup: Use only find_package to find dependencies. (#2967)
* feat: Add ability to disable building libraries (#3025)
* bug: fix builds with CMake 3.15 (#3033)
* feat: Document behavior of passing empty string to SetEnv on Windows (#3030)

## v0.10.x - 2019-08

* feat: add `conjunction` metafunction (#2892)
* bug: Fix typo in testing_util/config.cmake.in (#2851)
* bug: Include 'IncludeGMock.cmake' in testing_util/config.cmake.in (#2848)

## v0.9.x - 2019-07

* feature: support `operator==` and `operator!=` for `StatusOr`.
* feature: support assignment from `Status` in `StatusOr`.
* feature: disable `optional<T>`'s converting constructor when
  `U == optional<T>`.

## v0.7.x - 2019-05

* Support move-only callables in `future<T>`
* Avoid `std::make_exception_ptr()` in `future_shared_state_base::abandon()`.

## v0.6.x - 2019-04

* Avoid `std::make_exception_ptr()` when building without exceptions.
* Removed the googleapis submodule. The build system now automatically
  downloads all deps.

## v0.5.x - 2019-03

* **Breaking change**: Make `google::cloud::optional::operator bool()` explicit.
* Add `google::cloud::optional` value conversions that match `std::optional`.
* Stop using grpc's `DO_NOT_USE` enum.
* Remove ciso646 includes to force traditional spellings.
* Change `std::endl` -> `"\n"`.
* Enforce formatting of `.cc` files.

## v0.4.x - 2019-02

* Fixed some documentation.
* **Breaking change**: Removed `StatusOr<void>`.
* Updated `StatusOr` documentation.
* Fixed some (minor) issues found by coverity.

## v0.3.x - 2019-01

* Support compiling with gcc-4.8.
* Fix `GCP_LOG()` macro so it works on platforms that define a `DEBUG`
  pre-processor symbol.
* Use different PRNG sequences for each backoff instance, previously all the
  clones of a backoff policy shared the same sequence.
* Workaround build problems with Xcode 7.3.

## v0.2.x - 2018-12

* Implement `google::cloud::future<T>` and `google::cloud::promise<T>` based on
  ISO/IEC TS 19571:2016, the "C++ Extensions for Concurrency" technical
  specification, also known as "futures with continuations".

## v0.1.x - 2018-11

* `google::cloud::optional<T>` an intentionally incomplete implementation of
  `std::optional<T>` to support C++11 and C++14 users.
* Applications can configure `google::cloud::LogSink` to enable logging in some
  of the libraries and to redirect the logs to their preferred destination.
  The libraries do not enable any logging by default, not even to `stderr`.
* `google::cloud::SetTerminateHandler()` allows applications compiled without
  exceptions, but using the APIs that rely on exceptions to report errors, to
  configure how the application terminates when an unrecoverable error is
  detected by the libraries.
