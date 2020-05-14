// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_ASYNCLOGGING_H
#define MUDUO_BASE_ASYNCLOGGING_H

#include <atomic>
#include <vector>

#include "muduo/base/BlockingQueue.h"
#include "muduo/base/BoundedBlockingQueue.h"
#include "muduo/base/CountDownLatch.h"
#include "muduo/base/LogStream.h"
#include "muduo/base/Mutex.h"
#include "muduo/base/Thread.h"

namespace muduo {

class AsyncLogging : noncopyable {  // no copyable */

 public:
  AsyncLogging(const string& basename, off_t rollSize, int flushInterval = 3);

  ~AsyncLogging() {
    if (running_) {
      stop();
    }
  }

  void append(const char* logline, int len);

  void start() {
    running_ = true;
    thread_.start();
    latch_.wait();
  }

  void stop() NO_THREAD_SAFETY_ANALYSIS {
    running_ = false;
    cond_.notify();
    thread_.join();
  }

 private:
  void threadFunc();

  // tempate size 4MB
  // buffer raw pointer
  typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer;
  // unique pointer of buffer in vector
  // used buffer pointer in vector
  typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
  // unique pointer  unique_ptr<Buffer>
  typedef BufferVector::value_type BufferPtr;

  // flush interval time of buffer
  const int flushInterval_;
  // running flag
  std::atomic<bool> running_;
  // file name thread name
  const string basename_;
  // rolling size of logger append file
  const off_t rollSize_;
  // thread info
  muduo::Thread thread_;
  // count down timmer
  muduo::CountDownLatch latch_;
  // mutex
  muduo::MutexLock mutex_;
  // condition
  muduo::Condition cond_ GUARDED_BY(mutex_);
  // cuttent buffer
  BufferPtr currentBuffer_ GUARDED_BY(mutex_);
  // backup buffer
  BufferPtr nextBuffer_ GUARDED_BY(mutex_);
  // filled buffer
  BufferVector buffers_ GUARDED_BY(mutex_);
};

}  // namespace muduo

#endif  // MUDUO_BASE_ASYNCLOGGING_H
