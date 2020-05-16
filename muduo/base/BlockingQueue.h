/*
 * @Author: Mengsen.Wang
 * @Date: 2020-05-16 17:55:04
 * @Last Modified by: Mengsen.Wang
 * @Last Modified time: 2020-05-16 17:55:32
 * @Description: encapsulation block queue
 */
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_BLOCKINGQUEUE_H
#define MUDUO_BASE_BLOCKINGQUEUE_H

#include <assert.h>

#include <deque>

#include "muduo/base/Condition.h"
#include "muduo/base/Mutex.h"

namespace muduo {

template <typename T>
class BlockingQueue : noncopyable {
 public:
  BlockingQueue() : mutex_(), notEmpty_(mutex_), queue_() {}

  void put(const T& x) {
    MutexLockGuard lock(mutex_);
    queue_.push_back(x);
    notEmpty_.notify();  // wait morphing saves us
    // http://www.domaigne.com/blog/computing/condvars-signal-with-mutex-locked-or-not/
  }

  void put(T&& x) {
    MutexLockGuard lock(mutex_);
    queue_.push_back(std::move(x));
    notEmpty_.notify();
  }

  T take() {
    MutexLockGuard lock(mutex_);
    // always use a while-loop, due to spurious wakeup
    while (queue_.empty()) {
      notEmpty_.wait();
    }
    assert(!queue_.empty());
    T front(std::move(queue_.front()));
    queue_.pop_front();
    return front;
  }

  size_t size() const {
    MutexLockGuard lock(mutex_);
    return queue_.size();
  }

 private:
  mutable MutexLock mutex_;
  Condition notEmpty_ GUARDED_BY(mutex_);
  std::deque<T> queue_ GUARDED_BY(mutex_);
};

}  // namespace muduo

#endif  // MUDUO_BASE_BLOCKINGQUEUE_H
