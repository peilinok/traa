#include <gtest/gtest.h>

#include "base/thread/waitable_future.h"

#include <thread>

using namespace traa::base;

TEST(waitable_future_test, get) {
  // invalid future
  {
    ffuture<int> future;
    waitable_future<int> waitable_res(std::move(future));

    EXPECT_EQ(waitable_res.get(9527), 9527);
  }

  // valid future
  {
    fpromise<int> promise;
    ffuture<int> future = promise.get_future();
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();

    std::thread t([&promise]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      promise.set_value(9527);
    });

    EXPECT_EQ(waitable_res.get(0), 9527);

    auto end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 900);

    t.join();
  }
}

TEST(waitable_future_test, get_for) {
  // invalid future
  {
    ffuture<int> future;
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(waitable_res.get_for(std::chrono::milliseconds(500), 9527), 9527);

    auto end = std::chrono::steady_clock::now();
    EXPECT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 400);
  }

  // valid future
  {
    fpromise<int> promise;
    ffuture<int> future = promise.get_future();
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();

    EXPECT_EQ(waitable_res.get_for(std::chrono::milliseconds(200), 0), 0);

    auto end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);

    start = std::chrono::steady_clock::now();
    std::thread t([&promise]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(800));
      promise.set_value(9527);
    });

    EXPECT_EQ(waitable_res.get_for(std::chrono::milliseconds(1000), 0), 9527);
    end = std::chrono::steady_clock::now();

    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 600);

    t.join();
  }
}

TEST(waitable_future_test, get_until) {
  // invalid future
  {
    ffuture<int> future;
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    auto timeout_time = start + std::chrono::milliseconds(1000);
    EXPECT_EQ(waitable_res.get_until(timeout_time, 9527), 9527);
    auto end = std::chrono::steady_clock::now();
    EXPECT_LT(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 1000);
  }

  // valid future
  {
    fpromise<int> promise;
    ffuture<int> future = promise.get_future();
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    auto timeout_time = start + std::chrono::milliseconds(200);
    EXPECT_EQ(waitable_res.get_until(timeout_time, 0), 0);

    auto end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);

    fpromise<void> promise2;
    ffuture<void> future2 = promise2.get_future();
    std::thread t([&promise, &promise2]() {
      promise2.set_value();
      std::this_thread::sleep_for(std::chrono::milliseconds(800));
      promise.set_value(9527);
    });

    future2.wait();

    start = std::chrono::steady_clock::now();
    timeout_time = start + std::chrono::milliseconds(1000);

    EXPECT_EQ(waitable_res.get_until(timeout_time, 0), 9527);

    end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 600);

    t.join();
  }
}

TEST(waitable_future_test, wait) {
  // invalid future
  {
    ffuture<int> future;
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    waitable_res.wait();
    auto end = std::chrono::steady_clock::now();
    EXPECT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 10);
  }

  // valid future
  {
    fpromise<int> promise;
    ffuture<int> future = promise.get_future();
    waitable_future<int> waitable_res(std::move(future));

    std::thread t([&promise]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      promise.set_value(9527);
    });

    auto start = std::chrono::steady_clock::now();
    waitable_res.wait();
    auto end = std::chrono::steady_clock::now();

    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 800);

    EXPECT_EQ(waitable_res.get(0), 9527);

    t.join();
  }
}

TEST(waitable_future_test, wait_for) {
  // invalid future
  {
    ffuture<int> future;
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(waitable_res.wait_for(std::chrono::milliseconds(1000)),
              waitable_future_status::invalid);

    auto end = std::chrono::steady_clock::now();
    EXPECT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 200);
  }

  // valid future
  {
    fpromise<int> promise;
    ffuture<int> future = promise.get_future();
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(waitable_res.wait_for(std::chrono::milliseconds(200)),
              waitable_future_status::timeout);

    auto end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);

    fpromise<void> promise2;
    ffuture<void> future2 = promise2.get_future();
    std::thread t([&promise, &promise2]() {
      promise2.set_value();
      std::this_thread::sleep_for(std::chrono::milliseconds(800));
      promise.set_value(9527);
    });

    future2.wait();

    start = std::chrono::steady_clock::now();
    EXPECT_EQ(waitable_res.wait_for(std::chrono::milliseconds(1000)),
              waitable_future_status::ready);
    end = std::chrono::steady_clock::now();

    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 600);

    EXPECT_EQ(waitable_res.get(0), 9527);

    t.join();
  }
}

TEST(waitable_future_test, wait_until) {
  // invalid future
  {
    ffuture<int> future;
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    auto timeout_time = start + std::chrono::milliseconds(100);
    EXPECT_EQ(waitable_res.wait_until(timeout_time), waitable_future_status::invalid);

    auto end = std::chrono::steady_clock::now();
    EXPECT_LE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 10);
  }

  // valid future
  {
    fpromise<int> promise;
    ffuture<int> future = promise.get_future();
    waitable_future<int> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    auto timeout_time = start + std::chrono::milliseconds(200);
    EXPECT_EQ(waitable_res.wait_until(timeout_time), waitable_future_status::timeout);

    auto end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);

    fpromise<void> promise2;
    ffuture<void> future2 = promise2.get_future();
    std::thread t([&promise, &promise2]() {
      promise2.set_value();
      std::this_thread::sleep_for(std::chrono::milliseconds(800));
      promise.set_value(9527);
    });

    future2.wait();

    start = std::chrono::steady_clock::now();
    timeout_time = start + std::chrono::milliseconds(1000);
    EXPECT_EQ(waitable_res.wait_until(timeout_time), waitable_future_status::ready);

    end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 600);

    EXPECT_EQ(waitable_res.get(0), 9527);

    t.join();
  }
}

TEST(waitable_future_test, valid) {
  // valid future
  {
    fpromise<int> promise;
    ffuture<int> future = promise.get_future();
    waitable_future<int> waitable_res(std::move(future));

    EXPECT_TRUE(waitable_res.valid());

    promise.set_value(9527);

    EXPECT_TRUE(waitable_res.valid());
  }

  // invalid future
  {
    ffuture<int> future;
    waitable_future<int> waitable_res(std::move(future));

    EXPECT_FALSE(waitable_res.valid());
  }
}

TEST(waitable_future_void_test, wait) {
  // invalid future
  {
    ffuture<void> future;
    waitable_future<void> waitable_res(std::move(future));

    waitable_res.wait();
  }

  // valid future
  {
    fpromise<void> promise;
    ffuture<void> future = promise.get_future();
    waitable_future<void> waitable_res(std::move(future));

    fpromise<void> promise2;
    ffuture<void> future2 = promise2.get_future();
    std::thread t([&promise, &promise2]() {
      promise2.set_value();
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      promise.set_value();
    });

    future2.wait();

    auto start = std::chrono::steady_clock::now();
    waitable_res.wait();
    auto end = std::chrono::steady_clock::now();

    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 900);

    t.join();
  }
}

TEST(waitable_future_void_test, wait_for) {
  // invalid future
  {
    ffuture<void> future;
    waitable_future<void> waitable_res(std::move(future));

    EXPECT_EQ(waitable_res.wait_for(std::chrono::milliseconds(100)),
              waitable_future_status::invalid);
  }

  // valid future
  {
    fpromise<void> promise;
    ffuture<void> future = promise.get_future();
    waitable_future<void> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    EXPECT_EQ(waitable_res.wait_for(std::chrono::milliseconds(200)),
              waitable_future_status::timeout);

    auto end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);

    fpromise<void> promise2;
    ffuture<void> future2 = promise2.get_future();
    std::thread t([&promise, &promise2]() {
      promise2.set_value();
      std::this_thread::sleep_for(std::chrono::milliseconds(800));
      promise.set_value();
    });

    future2.wait();

    start = std::chrono::steady_clock::now();
    EXPECT_EQ(waitable_res.wait_for(std::chrono::milliseconds(1000)),
              waitable_future_status::ready);
    end = std::chrono::steady_clock::now();

    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 600);

    t.join();
  }
}

TEST(waitable_future_void_test, wait_until) {
  // invalid future
  {
    ffuture<void> future;
    waitable_future<void> waitable_res(std::move(future));

    auto timeout_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);
    EXPECT_EQ(waitable_res.wait_until(timeout_time), waitable_future_status::invalid);
  }

  // valid future
  {
    fpromise<void> promise;
    ffuture<void> future = promise.get_future();
    waitable_future<void> waitable_res(std::move(future));

    auto start = std::chrono::steady_clock::now();
    auto timeout_time = start + std::chrono::milliseconds(200);
    EXPECT_EQ(waitable_res.wait_until(timeout_time), waitable_future_status::timeout);

    auto end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 150);

    fpromise<void> promise2;
    ffuture<void> future2 = promise2.get_future();
    std::thread t([&promise, &promise2]() {
      promise2.set_value();
      std::this_thread::sleep_for(std::chrono::milliseconds(800));
      promise.set_value();
    });

    future2.wait();

    start = std::chrono::steady_clock::now();
    timeout_time = start + std::chrono::milliseconds(1000);
    EXPECT_EQ(waitable_res.wait_until(timeout_time), waitable_future_status::ready);

    end = std::chrono::steady_clock::now();
    EXPECT_GE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count(), 600);

    t.join();
  }
}

TEST(waitable_future_void_test, valid) {
  // valid future
  {
    fpromise<void> promise;
    ffuture<void> future = promise.get_future();
    waitable_future<void> waitable_res(std::move(future));

    EXPECT_TRUE(waitable_res.valid());

    promise.set_value();

    EXPECT_TRUE(waitable_res.valid());
  }

  // invalid future
  {
    ffuture<void> future;
    waitable_future<void> waitable_res(std::move(future));

    EXPECT_FALSE(waitable_res.valid());
  }
}