// Copyright 2018 Google LLC.
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

#include "google/cloud/internal/async_retry_unary_rpc.h"
#include "google/cloud/internal/backoff_policy.h"
#include "google/cloud/internal/retry_policy.h"
#include "google/cloud/testing_util/assert_ok.h"
#include "google/cloud/testing_util/chrono_literals.h"
#include <google/bigtable/admin/v2/bigtable_table_admin.grpc.pb.h>
#include <gmock/gmock.h>
#include <thread>

namespace google {
namespace cloud {
inline namespace GOOGLE_CLOUD_CPP_NS {
namespace internal {
namespace {

namespace btadmin = ::google::bigtable::admin::v2;
using ::testing::_;
using ::testing::Invoke;

// Tests typically create an instance of this class, then create a
// `google::cloud::bigtable::CompletionQueue` to wrap it, keeping a reference to
// the instance to manipulate its state directly.
class MockCompletionQueue
    : public google::cloud::internal::CompletionQueueImpl {
 public:
  std::unique_ptr<grpc::Alarm> CreateAlarm() const override {
    // grpc::Alarm objects are really hard to cleanup when mocking their
    // behavior, so we do not create an alarm, instead we return nullptr, which
    // the classes that care (AsyncTimerFunctor) know what to do with.
    return std::unique_ptr<grpc::Alarm>();
  }

  using CompletionQueueImpl::empty;
  using CompletionQueueImpl::SimulateCompletion;
  using CompletionQueueImpl::size;
};

/**
 * Define the interface to mock the result of starting a unary async RPC.
 *
 * Note that using this mock often requires special memory management. The
 * google mock library requires all mocks to be destroyed. In contrast, grpc
 * specializes `std::unique_ptr<>` to *not* delete objects of type
 * `grpc::ClientAsyncResponseReaderInterface<T>`:
 *
 *
 *     https://github.com/grpc/grpc/blob/608188c680961b8506847c135b5170b41a9081e8/include/grpcpp/impl/codegen/async_unary_call.h#L305
 *
 * No delete, no destructor, nothing. The gRPC library expects all
 * `grpc::ClientAsyncResponseReader<R>` objects to be allocated from a
 * per-call arena, and deleted in bulk with other objects when the call
 * completes and the full arena is released. Unfortunately, our mocks are
 * allocated from the global heap, as they do not have an associated call or
 * arena. The override in the gRPC library results in a leak, unless we manage
 * the memory explicitly.
 *
 * As a result, the unit tests need to manually delete the objects. The idiom we
 * use is a terrible, horrible, no good, very bad hack:
 *
 * We create a unique pointer to `MockAsyncResponseReader<T>`, then we pass that
 * pointer to gRPC using `reader.get()`, and gRPC promptly puts it into a
 * `std::unique_ptr<ClientAsyncResponseReaderInterface<T>>`. That looks like a
 * double delete waiting to happen, but it is not, because gRPC has done the
 * weird specialization of `std::unique_ptr`.
 *
 * @tparam Response the type of the RPC response
 */
template <typename Response>
class MockAsyncResponseReader
    : public grpc::ClientAsyncResponseReaderInterface<Response> {
 public:
  MOCK_METHOD0(StartCall, void());
  MOCK_METHOD1(ReadInitialMetadata, void(void*));
  MOCK_METHOD3_T(Finish, void(Response*, grpc::Status*, void*));
};

class MockStub {
 public:
  MOCK_METHOD3(
      AsyncGetTable,
      std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<btadmin::Table>>(
          grpc::ClientContext*, btadmin::GetTableRequest const&,
          grpc::CompletionQueue* cq));

  MOCK_METHOD3(AsyncDeleteTable,
               std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<
                   google::protobuf::Empty>>(grpc::ClientContext*,
                                             btadmin::DeleteTableRequest const&,
                                             grpc::CompletionQueue* cq));
};

struct IsRetryableTraits {
  static bool IsPermanentFailure(Status const& status) {
    return !status.ok() && status.code() != StatusCode::kUnavailable;
  }
};

using RpcRetryPolicy =
    google::cloud::internal::RetryPolicy<Status, IsRetryableTraits>;
using RpcLimitedErrorCountRetryPolicy =
    google::cloud::internal::LimitedErrorCountRetryPolicy<Status,
                                                          IsRetryableTraits>;
using RpcBackoffPolicy = google::cloud::internal::BackoffPolicy;
using RpcExponentialBackoffPolicy =
    google::cloud::internal::ExponentialBackoffPolicy;

TEST(AsyncRetryUnaryRpcTest, ImmediatelySucceeds) {
  using namespace google::cloud::testing_util::chrono_literals;

  MockStub mock;

  using ReaderType = MockAsyncResponseReader<btadmin::Table>;
  auto reader = google::cloud::internal::make_unique<ReaderType>();
  EXPECT_CALL(*reader, Finish(_, _, _))
      .WillOnce(Invoke([](btadmin::Table* table, grpc::Status* status, void*) {
        // Initialize a value to make sure it is carried all the way back to
        // the caller.
        table->set_name("fake/table/name/response");
        *status = grpc::Status::OK;
      }));

  EXPECT_CALL(mock, AsyncGetTable(_, _, _))
      .WillOnce(Invoke([&reader](grpc::ClientContext*,
                                 btadmin::GetTableRequest const& request,
                                 grpc::CompletionQueue*) {
        EXPECT_EQ("fake/table/name/request", request.name());
        return std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<
            // This is safe, see comments in MockAsyncResponseReader.
            btadmin::Table>>(reader.get());
      }));

  auto impl = std::make_shared<MockCompletionQueue>();
  CompletionQueue cq(impl);

  // Do some basic initialization of the request to verify the values get
  // carried to the mock.
  btadmin::GetTableRequest request;
  request.set_name("fake/table/name/request");

  auto async_call = [&mock](grpc::ClientContext* context,
                            btadmin::GetTableRequest const& request,
                            grpc::CompletionQueue* cq) {
    return mock.AsyncGetTable(context, request, cq);
  };

  static_assert(google::cloud::internal::is_invocable<
                    decltype(async_call), grpc::ClientContext*,
                    decltype(request), grpc::CompletionQueue*>::value,
                "");

  auto fut = StartRetryAsyncUnaryRpc(
      cq, __func__, RpcLimitedErrorCountRetryPolicy(3).clone(),
      RpcExponentialBackoffPolicy(10_us, 40_us, 2.0).clone(),
      /*is_idempotent=*/true, async_call, request);

  EXPECT_EQ(1, impl->size());
  impl->SimulateCompletion(true);

  EXPECT_TRUE(impl->empty());
  EXPECT_EQ(std::future_status::ready, fut.wait_for(0_us));
  auto result = fut.get();
  ASSERT_STATUS_OK(result);
  EXPECT_EQ("fake/table/name/response", result->name());
}

TEST(AsyncRetryUnaryRpcTest, VoidImmediatelySucceeds) {
  using namespace google::cloud::testing_util::chrono_literals;

  MockStub mock;

  using ReaderType = MockAsyncResponseReader<google::protobuf::Empty>;
  auto reader = google::cloud::internal::make_unique<ReaderType>();
  EXPECT_CALL(*reader, Finish(_, _, _))
      .WillOnce(Invoke([](google::protobuf::Empty*, grpc::Status* status,
                          void*) { *status = grpc::Status::OK; }));

  EXPECT_CALL(mock, AsyncDeleteTable(_, _, _))
      .WillOnce(Invoke([&reader](grpc::ClientContext*,
                                 btadmin::DeleteTableRequest const& request,
                                 grpc::CompletionQueue*) {
        EXPECT_EQ("fake/table/name/request", request.name());
        return std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<
            // This is safe, see comments in MockAsyncResponseReader.
            google::protobuf::Empty>>(reader.get());
      }));

  auto impl = std::make_shared<MockCompletionQueue>();
  CompletionQueue cq(impl);

  // Do some basic initialization of the request to verify the values get
  // carried to the mock.
  btadmin::DeleteTableRequest request;
  request.set_name("fake/table/name/request");

  auto async_call = [&mock](grpc::ClientContext* context,
                            btadmin::DeleteTableRequest const& request,
                            grpc::CompletionQueue* cq) {
    return mock.AsyncDeleteTable(context, request, cq);
  };

  static_assert(google::cloud::internal::is_invocable<
                    decltype(async_call), grpc::ClientContext*,
                    decltype(request), grpc::CompletionQueue*>::value,
                "");

  auto fut = StartRetryAsyncUnaryRpc(
      cq, __func__, RpcLimitedErrorCountRetryPolicy(3).clone(),
      RpcExponentialBackoffPolicy(10_us, 40_us, 2.0).clone(),
      /*is_idempotent=*/true, async_call, request);

  EXPECT_EQ(1, impl->size());
  impl->SimulateCompletion(true);

  EXPECT_TRUE(impl->empty());
  EXPECT_EQ(std::future_status::ready, fut.wait_for(0_us));
  auto result = fut.get();
  ASSERT_STATUS_OK(result);
}

#if 0

TEST(AsyncRetryUnaryRpcTest, PermanentFailure) {
  using namespace google::cloud::testing_util::chrono_literals;

  MockClient client;

  using ReaderType =
      ::google::cloud::bigtable::testing::MockAsyncResponseReader<
          btadmin::Table>;
  auto reader = google::cloud::internal::make_unique<ReaderType>();
  EXPECT_CALL(*reader, Finish(_, _, _))
      .WillOnce(Invoke([](btadmin::Table*, grpc::Status* status, void*) {
        *status = grpc::Status(grpc::StatusCode::PERMISSION_DENIED, "uh-oh");
      }));

  EXPECT_CALL(client, AsyncGetTable(_, _, _))
      .WillOnce(Invoke([&reader](grpc::ClientContext*,
                                 btadmin::GetTableRequest const& request,
                                 grpc::CompletionQueue*) {
        EXPECT_EQ("fake/table/name/request", request.name());
        return std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<
            // This is safe, see comments in MockAsyncResponseReader.
            btadmin::Table>>(reader.get());
      }));

  auto impl = std::make_shared<testing::MockCompletionQueue>();
  bigtable::CompletionQueue cq(impl);

  // Do some basic initialization of the request to verify the values get
  // carried to the mock.
  btadmin::GetTableRequest request;
  request.set_name("fake/table/name/request");

  auto fut = StartRetryAsyncUnaryRpc(
      __func__, LimitedErrorCountRetryPolicy(3).clone(),
      ExponentialBackoffPolicy(10_us, 40_us).clone(),
      ConstantIdempotencyPolicy(true),
      MetadataUpdatePolicy("resource", MetadataParamTypes::RESOURCE),
      [&client](grpc::ClientContext* context,
                btadmin::GetTableRequest const& request,
                grpc::CompletionQueue* cq) {
        return client.AsyncGetTable(context, request, cq);
      },
      request, cq);

  EXPECT_EQ(1, impl->size());
  impl->SimulateCompletion(true);

  EXPECT_TRUE(impl->empty());
  EXPECT_EQ(std::future_status::ready, fut.wait_for(0_us));
  auto result = fut.get();
  EXPECT_FALSE(result);
  EXPECT_EQ(StatusCode::kPermissionDenied, result.status().code());
}

TEST(AsyncRetryUnaryRpcTest, TooManyTransientFailures) {
  using namespace google::cloud::testing_util::chrono_literals;

  MockClient client;

  using ReaderType =
      ::google::cloud::bigtable::testing::MockAsyncResponseReader<
          btadmin::Table>;

  auto finish_failure = [](btadmin::Table*, grpc::Status* status, void*) {
    *status = grpc::Status(grpc::StatusCode::UNAVAILABLE, "try-again");
  };

  auto r1 = google::cloud::internal::make_unique<ReaderType>();
  EXPECT_CALL(*r1, Finish(_, _, _)).WillOnce(Invoke(finish_failure));
  auto r2 = google::cloud::internal::make_unique<ReaderType>();
  EXPECT_CALL(*r2, Finish(_, _, _)).WillOnce(Invoke(finish_failure));
  auto r3 = google::cloud::internal::make_unique<ReaderType>();
  EXPECT_CALL(*r3, Finish(_, _, _)).WillOnce(Invoke(finish_failure));

  EXPECT_CALL(client, AsyncGetTable(_, _, _))
      .WillOnce(Invoke([&r1](grpc::ClientContext*,
                             btadmin::GetTableRequest const& request,
                             grpc::CompletionQueue*) {
        EXPECT_EQ("fake/table/name/request", request.name());
        return std::unique_ptr<
            grpc::ClientAsyncResponseReaderInterface<btadmin::Table>>(r1.get());
      }))
      .WillOnce(Invoke([&r2](grpc::ClientContext*,
                             btadmin::GetTableRequest const& request,
                             grpc::CompletionQueue*) {
        EXPECT_EQ("fake/table/name/request", request.name());
        return std::unique_ptr<
            grpc::ClientAsyncResponseReaderInterface<btadmin::Table>>(r2.get());
      }))
      .WillOnce(Invoke([&r3](grpc::ClientContext*,
                             btadmin::GetTableRequest const& request,
                             grpc::CompletionQueue*) {
        EXPECT_EQ("fake/table/name/request", request.name());
        return std::unique_ptr<
            grpc::ClientAsyncResponseReaderInterface<btadmin::Table>>(r3.get());
      }));

  auto impl = std::make_shared<testing::MockCompletionQueue>();
  bigtable::CompletionQueue cq(impl);

  // Do some basic initialization of the request to verify the values get
  // carried to the mock.
  btadmin::GetTableRequest request;
  request.set_name("fake/table/name/request");

  auto fut = StartRetryAsyncUnaryRpc(
      __func__, LimitedErrorCountRetryPolicy(2).clone(),
      ExponentialBackoffPolicy(10_us, 40_us).clone(),
      ConstantIdempotencyPolicy(true),
      MetadataUpdatePolicy("resource", MetadataParamTypes::RESOURCE),
      [&client](grpc::ClientContext* context,
                btadmin::GetTableRequest const& request,
                grpc::CompletionQueue* cq) {
        return client.AsyncGetTable(context, request, cq);
      },
      request, cq);

  // Because the maximum number of failures is 2 we expect 3 calls (the 3rd
  // failure is the "too many" case). In between the calls there are timers
  // executed, but there is no timer after the 3rd failure.
  EXPECT_EQ(1, impl->size());  // simulate the call completing
  impl->SimulateCompletion(true);
  EXPECT_EQ(1, impl->size());  // simulate the timer completing
  impl->SimulateCompletion(true);
  EXPECT_EQ(1, impl->size());  // simulate the call completing
  impl->SimulateCompletion(true);
  EXPECT_EQ(1, impl->size());  // simulate the timer completing
  impl->SimulateCompletion(true);
  EXPECT_EQ(1, impl->size());  // simulate the call completing
  impl->SimulateCompletion(true);
  EXPECT_TRUE(impl->empty());

  EXPECT_EQ(std::future_status::ready, fut.wait_for(0_us));
  auto result = fut.get();
  EXPECT_FALSE(result);
  EXPECT_EQ(StatusCode::kUnavailable, result.status().code());
}

#endif
}  // namespace
}  // namespace internal
}  // namespace GOOGLE_CLOUD_CPP_NS
}  // namespace cloud
}  // namespace google
