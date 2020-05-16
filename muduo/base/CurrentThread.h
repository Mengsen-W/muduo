/*
 * @Author: Mengsen.Wang
 * @Date: 2020-05-16 18:23:44
 * @Last Modified by: Mengsen.Wang
 * @Last Modified time: 2020-05-16 18:28:24
 * @Description: recoder thread state
 */

// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_CURRENTTHREAD_H
#define MUDUO_BASE_CURRENTTHREAD_H

#include "muduo/base/Types.h"

namespace muduo {
namespace CurrentThread {
// internal
extern __thread int t_cachedTid;
extern __thread char t_tidString[32];
extern __thread int t_tidStringLength;
extern __thread const char* t_threadName;
void cacheTid();

inline int tid() {
  // tell the compiler which branch is most likely to be executed
  if (__builtin_expect(t_cachedTid == 0, 0)) {
    cacheTid();
  }
  return t_cachedTid;
}

// for logging
inline const char* tidString() { return t_tidString; }

// for logging
inline int tidStringLength() { return t_tidStringLength; }

inline const char* name() { return t_threadName; }

bool isMainThread();

void sleepUsec(int64_t usec);  // for testing

string stackTrace(bool demangle);
}  // namespace CurrentThread
}  // namespace muduo

#endif  // MUDUO_BASE_CURRENTTHREAD_H
