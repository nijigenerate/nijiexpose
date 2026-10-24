#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <mutex>
#include <optional>
#include <thread>
#include <utility>

namespace niji {
enum class Delivery { ordered, latest };
enum class ReadStatus { value, timeout, end };

// One owned item, with either backpressure (files) or replacement (live input).
// No OS thread API and no unbounded queue. All waits are interruptible by cancel.
template<class T> class Channel {
  const Delivery delivery_;
  mutable std::mutex mutex_;
  std::condition_variable readable_, writable_;
  std::optional<T> item_;
  bool finished_ = false, cancelled_ = false;
  std::exception_ptr error_;
  std::uint64_t published_ = 0, replaced_ = 0;
public:
  explicit Channel(Delivery delivery) : delivery_(delivery) {}
  bool send(T item) {
    std::unique_lock lock(mutex_);
    if (delivery_ == Delivery::ordered)
      writable_.wait(lock, [&] { return finished_ || !item_; });
    if (finished_) return false;
    if (item_) ++replaced_;
    item_ = std::move(item);
    ++published_;
    lock.unlock();
    readable_.notify_one();
    return true;
  }
  ReadStatus receive(T &item, std::chrono::milliseconds timeout) {
    std::unique_lock lock(mutex_);
    if (!readable_.wait_for(lock, timeout, [&] { return item_ || finished_; }))
      return ReadStatus::timeout;
    if (item_) {
      item = std::move(*item_);
      item_.reset();
      lock.unlock();
      writable_.notify_one();
      return ReadStatus::value;
    }
    if (error_) std::rethrow_exception(error_);
    return ReadStatus::end;
  }
  void finish(std::exception_ptr error = {}) {
    {
      std::lock_guard lock(mutex_);
      if (!cancelled_) error_ = error;
      finished_ = true;
    }
    readable_.notify_all(); writable_.notify_all();
  }
  void cancel() {
    {
      std::lock_guard lock(mutex_);
      cancelled_ = finished_ = true;
      item_.reset(); error_ = {};
    }
    readable_.notify_all(); writable_.notify_all();
  }
  std::uint64_t published() const { std::lock_guard lock(mutex_); return published_; }
  std::uint64_t replaced() const { std::lock_guard lock(mutex_); return replaced_; }
};

// Owns and joins its thread, including when the consumer throws or closes a GUI.
// The producer must return from external I/O before join can finish.
template<class T> class WorkerStream {
  Channel<T> channel_;
  std::atomic<bool> stop_{false};
  std::thread thread_;
public:
  template<class Task> WorkerStream(Delivery delivery, Task task) : channel_(delivery) {
    thread_ = std::thread([this, task = std::move(task)]() mutable {
      try { task(*this); channel_.finish(); }
      catch (...) { channel_.finish(std::current_exception()); }
    });
  }
  ~WorkerStream() { request_stop(); join(); }
  WorkerStream(const WorkerStream &) = delete;
  WorkerStream &operator=(const WorkerStream &) = delete;
  bool stopped() const { return stop_.load(std::memory_order_relaxed); }
  bool publish(T item) { return channel_.send(std::move(item)); }
  ReadStatus next(T &item, std::chrono::milliseconds timeout) { return channel_.receive(item, timeout); }
  void request_stop() { stop_.store(true, std::memory_order_relaxed); channel_.cancel(); }
  void join() { if (thread_.joinable()) thread_.join(); }
  std::uint64_t published() const { return channel_.published(); }
  std::uint64_t dropped() const { return channel_.replaced(); }
};
} // namespace niji
