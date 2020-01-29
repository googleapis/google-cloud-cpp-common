// Copyright 2019 Google LLC
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

#include "google/cloud/grpc_utils/completion_queue.h"
#include "google/cloud/future.h"
#include "google/cloud/testing_util/assert_ok.h"
#include <google/bigtable/admin/v2/bigtable_table_admin.grpc.pb.h>
#include <google/bigtable/v2/bigtable.grpc.pb.h>
#include <gmock/gmock.h>
#include <chrono>
#include <memory>
#include <thread>

namespace google {
namespace cloud {
namespace grpc_utils {
inline namespace GOOGLE_CLOUD_CPP_GRPC_UTILS_NS {
namespace {

class MockCompletionQueue : public internal::CompletionQueueImpl {
 public:
  using internal::CompletionQueueImpl::SimulateCompletion;
};

namespace btadmin = ::google::bigtable::admin::v2;
namespace btproto = ::google::bigtable::v2;
using ::testing::_;
using ::testing::Invoke;

class MockClient {
 public:
  MOCK_METHOD3(
      AsyncGetTable,
      std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<btadmin::Table>>(
          grpc::ClientContext*, btadmin::GetTableRequest const&,
          grpc::CompletionQueue* cq));

  MOCK_METHOD3(AsyncReadRows,
               std::unique_ptr<::grpc::ClientAsyncReaderInterface<
                   btproto::ReadRowsResponse>>(grpc::ClientContext*,
                                               btproto::ReadRowsRequest const&,
                                               grpc::CompletionQueue* cq));
};

class MockTableReader
    : public grpc::ClientAsyncResponseReaderInterface<btadmin::Table> {
 public:
  MOCK_METHOD0(StartCall, void());
  MOCK_METHOD1(ReadInitialMetadata, void(void*));
  MOCK_METHOD3(Finish, void(btadmin::Table*, grpc::Status*, void*));
};

class MockRowReader
    : public grpc::ClientAsyncReaderInterface<btproto::ReadRowsResponse> {
 public:
  MOCK_METHOD1(StartCall, void(void*));
  MOCK_METHOD1(ReadInitialMetadata, void(void*));
  MOCK_METHOD2(Read, void(btproto::ReadRowsResponse*, void*));
  MOCK_METHOD2(Finish, void(grpc::Status*, void*));
};

/// @test Verify that the basic functionality in a CompletionQueue works.
TEST(CompletionQueueTest, TimerSmokeTest) {
  CompletionQueue cq;
  std::thread t([&cq] { cq.Run(); });

  using ms = std::chrono::milliseconds;
  promise<void> wait_for_sleep;
  cq.MakeRelativeTimer(ms(2))
      .then([&wait_for_sleep](
                future<StatusOr<std::chrono::system_clock::time_point>>) {
        wait_for_sleep.set_value();
      })
      .get();

  auto f = wait_for_sleep.get_future();
  EXPECT_EQ(std::future_status::ready, f.wait_for(ms(0)));
  cq.Shutdown();
  t.join();
}

TEST(CompletionQueueTest, MockSmokeTest) {
  auto mock = std::make_shared<MockCompletionQueue>();

  CompletionQueue cq(mock);
  using ms = std::chrono::milliseconds;
  promise<void> wait_for_sleep;
  cq.MakeRelativeTimer(ms(20000)).then(
      [&wait_for_sleep](
          future<StatusOr<std::chrono::system_clock::time_point>>) {
        wait_for_sleep.set_value();
      });
  mock->SimulateCompletion(/*ok=*/true);

  auto f = wait_for_sleep.get_future();
  EXPECT_EQ(std::future_status::ready, f.wait_for(ms(0)));
  cq.Shutdown();
}

TEST(CompletionQueueTest, ShutdownWithPending) {
  using ms = std::chrono::milliseconds;

  future<void> timer;
  {
    CompletionQueue cq;
    std::thread runner([&cq] { cq.Run(); });
    timer = cq.MakeRelativeTimer(ms(20)).then(
        [](future<StatusOr<std::chrono::system_clock::time_point>> result) {
          // Timer still runs to completion after `Shutdown`.
          EXPECT_STATUS_OK(result.get().status());
        });
    EXPECT_EQ(std::future_status::timeout, timer.wait_for(ms(0)));
    cq.Shutdown();
    EXPECT_EQ(std::future_status::timeout, timer.wait_for(ms(0)));
    runner.join();
  }
  EXPECT_EQ(std::future_status::ready, timer.wait_for(ms(0)));
}

TEST(CompletionQueueTest, CanCancelAllEvents) {
  using ms = std::chrono::milliseconds;

  CompletionQueue cq;
  promise<void> done;
  std::thread runner([&cq, &done] {
    cq.Run();
    done.set_value();
  });
  for (int i = 0; i < 3; ++i) {
    cq.MakeRelativeTimer(ms(20000)).then(
        [](future<StatusOr<std::chrono::system_clock::time_point>> result) {
          // Cancelled timers return CANCELLED status.
          EXPECT_EQ(StatusCode::kCancelled, result.get().status().code());
        });
  }
  auto f = done.get_future();
  EXPECT_EQ(std::future_status::timeout, f.wait_for(ms(1)));
  cq.Shutdown();
  EXPECT_EQ(std::future_status::timeout, f.wait_for(ms(1)));
  cq.CancelAll();
  EXPECT_EQ(std::future_status::ready, f.wait_for(ms(100)));
  runner.join();
}

TEST(CompletionQueueTest, MakeUnaryRpc) {
  using ms = std::chrono::milliseconds;

  auto mock_cq = std::make_shared<MockCompletionQueue>();
  CompletionQueue cq(mock_cq);

  auto mock_reader = google::cloud::internal::make_unique<MockTableReader>();
  EXPECT_CALL(*mock_reader, Finish(_, _, _))
      .WillOnce([](btadmin::Table* table, grpc::Status* status, void*) {
        table->set_name("test-table-name");
        *status = grpc::Status::OK;
      });
  MockClient mock_client;
  EXPECT_CALL(mock_client, AsyncGetTable(_, _, _))
      .WillOnce([&mock_reader](grpc::ClientContext*,
                               btadmin::GetTableRequest const& request,
                               grpc::CompletionQueue*) {
        EXPECT_EQ("test-table-name", request.name());
        // This looks like a double delete, but it is not because
        // std::unique_ptr<grpc::ClientAsyncResponseReaderInterface<T>> is
        // specialized to not delete. :shrug:
        return std::unique_ptr<
            grpc::ClientAsyncResponseReaderInterface<btadmin::Table>>(
            mock_reader.get());
      });

  std::thread runner([&cq] { cq.Run(); });

  btadmin::GetTableRequest request;
  request.set_name("test-table-name");
  future<void> done =
      cq.MakeUnaryRpc(
            [&mock_client](grpc::ClientContext* context,
                           btadmin::GetTableRequest const& request,
                           grpc::CompletionQueue* cq) {
              return mock_client.AsyncGetTable(context, request, cq);
            },
            request,
            google::cloud::internal::make_unique<grpc::ClientContext>())
          .then([](future<StatusOr<btadmin::Table>> f) {
            auto table = f.get();
            ASSERT_STATUS_OK(table);
            EXPECT_EQ("test-table-name", table->name());
          });

  mock_cq->SimulateCompletion(true);

  EXPECT_EQ(std::future_status::ready, done.wait_for(ms(0)));

  cq.Shutdown();
  runner.join();
}

TEST(CompletionQueueTest, MakeStreamingReadRpc) {
  auto mock_cq = std::make_shared<MockCompletionQueue>();
  CompletionQueue cq(mock_cq);

  auto mock_reader = google::cloud::internal::make_unique<MockRowReader>();
  EXPECT_CALL(*mock_reader, StartCall(_)).Times(1);
  EXPECT_CALL(*mock_reader, Read(_, _)).Times(2);
  EXPECT_CALL(*mock_reader, Finish(_, _)).Times(1);

  MockClient mock_client;
  EXPECT_CALL(mock_client, AsyncReadRows(_, _, _))
      .WillOnce([&mock_reader](grpc::ClientContext*,
                               btproto::ReadRowsRequest const& request,
                               grpc::CompletionQueue*) {
        EXPECT_EQ("test-table-name", request.table_name());
        return std::unique_ptr<
            grpc::ClientAsyncReaderInterface<btproto::ReadRowsResponse>>(
            mock_reader.release());
      });

  std::thread runner([&cq] { cq.Run(); });

  btproto::ReadRowsRequest request;
  request.set_table_name("test-table-name");

  int on_read_counter = 0;
  int on_finish_counter = 0;
  (void)cq.MakeStreamingReadRpc(
      [&mock_client](grpc::ClientContext* context,
                     btproto::ReadRowsRequest const& request,
                     grpc::CompletionQueue* cq) {
        return mock_client.AsyncReadRows(context, request, cq);
      },
      request, google::cloud::internal::make_unique<grpc::ClientContext>(),
      [&on_read_counter](btproto::ReadRowsResponse const&) {
        ++on_read_counter;
        return make_ready_future(true);
      },
      [&on_finish_counter](Status const&) { ++on_finish_counter; });

  // Simulate the OnStart() completion
  mock_cq->SimulateCompletion(true);
  // Simulate the first Read() completion
  mock_cq->SimulateCompletion(true);
  EXPECT_EQ(1, on_read_counter);
  EXPECT_EQ(0, on_finish_counter);

  // Simulate a Read() returning false
  mock_cq->SimulateCompletion(false);
  EXPECT_EQ(1, on_read_counter);
  EXPECT_EQ(0, on_finish_counter);

  // Simulate the Finish() call completing asynchronously
  mock_cq->SimulateCompletion(false);
  EXPECT_EQ(1, on_read_counter);
  EXPECT_EQ(1, on_finish_counter);

  cq.Shutdown();
  runner.join();
}

}  // namespace
}  // namespace GOOGLE_CLOUD_CPP_GRPC_UTILS_NS
}  // namespace grpc_utils
}  // namespace cloud
}  // namespace google
