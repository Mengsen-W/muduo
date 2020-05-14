// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/AsyncLogging.h"

#include <stdio.h>

#include "muduo/base/LogFile.h"
#include "muduo/base/Timestamp.h"

using namespace muduo;
/**
 * Brief: constructor
 * Param: const string& basename, off_t rollSize
 * int flushInterval
 */
AsyncLogging::AsyncLogging(const string& basename, off_t rollSize,
                           int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(basename),
      rollSize_(rollSize),
      // 'this' is first parameter of member function
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      latch_(1),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_() {
  // memset 0 to currentBuffer
  currentBuffer_->bzero();
  // memset 0 to nextBuffer
  nextBuffer_->bzero();
  // init 16 bucket of vector
  buffers_.reserve(16);
}

/**
 * @Brief: This function is called by front end
 * which send processed message to buffer
 * @Param: const char *logline, int len
 * @Return: void
 */
void AsyncLogging::append(const char* logline, int len) {
  // lock logger mutex
  muduo::MutexLockGuard lock(mutex_);

  if (currentBuffer_->avail() > len) {
    // current buffer not full
    currentBuffer_->append(logline, len);
  } else {
    // current buffer if full, push it and find next spare buffer
    buffers_.push_back(std::move(currentBuffer_));

    if (nextBuffer_) {
      // move backup buffer
      currentBuffer_ = std::move(nextBuffer_);
    } else {
      // Rarely happens
      currentBuffer_.reset(new Buffer);
    }
    currentBuffer_->append(logline, len);
    // notify back end to write message
    // maybe back end negative write mssage
    cond_.notify();
  }
}

/**
 * @Brief: logger thread function
 * @Param: void
 * @Return: void
 */
void AsyncLogging::threadFunc() {
  // judge running flag
  assert(running_ == true);
  // count down timer
  latch_.countDown();
  // create a new object
  LogFile output(basename_, rollSize_, false);

  // init buffer
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  newBuffer1->bzero();
  newBuffer2->bzero();

  // init buffer vector
  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);

  while (running_) {
    // judge running flag
    assert(newBuffer1 && newBuffer1->length() == 0);
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());

    {  // critical region
      muduo::MutexLockGuard lock(mutex_);
      if (buffers_.empty()) {
        // unusual usage just in thread begin
        // timeout in flush interval
        // front end write full more than one buffer
        cond_.waitForSeconds(flushInterval_);
      }
      // condition satisfy or timeout
      // force flush currentBuffer
      buffers_.push_back(std::move(currentBuffer_));
      // swap newbuffer with currentBuffer
      currentBuffer_ = std::move(newBuffer1);
      // force flush buffers
      buffersToWrite.swap(buffers_);
      if (!nextBuffer_) {
        // if next buffer move to current buffer
        // lead next buffer to failure
        nextBuffer_ = std::move(newBuffer2);
      }
    }  // exit critical section

    assert(!buffersToWrite.empty());

    if (buffersToWrite.size() > 25) {
      // so many to write
      // maybe over 25k log in buffer vector
      // must be adjust interval timeout
      // it is a self-protection mechanism
      char buf[256];
      snprintf(buf, sizeof buf,
               "Dropped log messages at %s, %zd larger buffers\n",
               Timestamp::now().toFormattedString().c_str(),
               buffersToWrite.size() - 2);
      // put waring to stderr
      fputs(buf, stderr);
      // write to log
      output.append(buf, static_cast<int>(strlen(buf)));
      // erase buffer vector expect for first two
      buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
    }

    for (const auto& buffer : buffersToWrite) {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      // write to log
      output.append(buffer->data(), buffer->length());
    }

    if (buffersToWrite.size() > 2) {
      // double check for buffer vector
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2);
    }

    if (!newBuffer1) {
      // reset new buffer but to
      // not new but move for fast
      assert(!buffersToWrite.empty());
      newBuffer1 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2) {
      assert(!buffersToWrite.empty());
      newBuffer2 = std::move(buffersToWrite.back());
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }

    // clear buffer vector
    buffersToWrite.clear();
    // flush output
    output.flush();
  }
  // for running close to flush output
  output.flush();
}
