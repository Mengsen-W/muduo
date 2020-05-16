/*
 * @Author: Mengsen.Wang
 * @Date: 2020-05-16 18:20:48
 * @Last Modified by:   Mengsen.Wang
 * @Last Modified time: 2020-05-16 18:20:48
 * @Description: count in order to wake up
 */

// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/CountDownLatch.h"

using namespace muduo;

CountDownLatch::CountDownLatch(int count)
    : mutex_(), condition_(mutex_), count_(count) {}

void CountDownLatch::wait() {
  MutexLockGuard lock(mutex_);
  while (count_ > 0) {
    condition_.wait();
  }
}

void CountDownLatch::countDown() {
  MutexLockGuard lock(mutex_);
  --count_;
  if (count_ == 0) {
    condition_.notifyAll();
  }
}

int CountDownLatch::getCount() const {
  MutexLockGuard lock(mutex_);
  return count_;
}
