/*
 * @Author: Mengsen.Wang
 * @Date: 2020-05-15 16:32:34
 * @Last Modified by: Mengsen.Wang
 * @Last Modified time: 2020-05-15 17:51:36
 * @Description: main logger
 */

// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_LOGGING_H
#define MUDUO_BASE_LOGGING_H

#include "muduo/base/LogStream.h"
#include "muduo/base/Timestamp.h"

namespace muduo {

class TimeZone;

class Logger {
 public:
  enum LogLevel {
    // log level
    TRACE,
    DEBUG,
    INFO,
    WARN,
    ERROR,
    FATAL,
    NUM_LOG_LEVELS,
  };

  // compile time calculation of basename of source file
  class SourceFile {
   public:
    template <int N>
    SourceFile(const char (&arr)[N]) : data_(arr), size_(N - 1) {
      // maybe no '/0' in the end
      // find last position of '/'
      const char* slash = strrchr(data_, '/');  // builtin function
      if (slash) {
        data_ = slash + 1;
        size_ -= static_cast<int>(data_ - arr);
      }
    }

    explicit SourceFile(const char* filename) : data_(filename) {
      const char* slash = strrchr(filename, '/');
      if (slash) {
        data_ = slash + 1;
      }
      size_ = static_cast<int>(strlen(data_));
    }

    const char* data_;  // before '/'
    int size_;          // no '/0'
  };

  Logger(SourceFile file, int line);
  Logger(SourceFile file, int line, LogLevel level);
  Logger(SourceFile file, int line, LogLevel level, const char* func);
  Logger(SourceFile file, int line, bool toAbort);
  ~Logger();

  LogStream& stream() { return impl_.stream_; }

  // return logLevel
  static LogLevel logLevel();
  // set loglevel
  static void setLogLevel(LogLevel level);

  // set function of output, flush, timezone
  typedef void (*OutputFunc)(const char* msg, int len);
  typedef void (*FlushFunc)();
  static void setOutput(OutputFunc);
  static void setFlush(FlushFunc);
  static void setTimeZone(const TimeZone& tz);

 private:
 // impl is a resource bundle
  class Impl {
   public:
    typedef Logger::LogLevel LogLevel;
    Impl(LogLevel level, int old_errno, const SourceFile& file, int line);
    void formatTime();
    void finish();

    // time stamp
    Timestamp time_;
    // logstream
    LogStream stream_;
    // log level
    LogLevel level_;
    // line of log
    int line_;
    // basename of source file
    SourceFile basename_;
  };

  Impl impl_;
};

extern Logger::LogLevel g_logLevel;

inline Logger::LogLevel Logger::logLevel() { return g_logLevel; }

//
// CAUTION: do not write:
//
// if (good)
//   LOG_INFO << "Good news";
// else
//   LOG_WARN << "Bad news";
//
// this expends to
//
// if (good)
//   if (logging_INFO)
//     logInfoStream << "Good news";
//   else
//     logWarnStream << "Bad news";
//

// compile period determine
#define LOG_TRACE                                        \
  if (muduo::Logger::logLevel() <= muduo::Logger::TRACE) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::TRACE, __func__).stream()
#define LOG_DEBUG                                        \
  if (muduo::Logger::logLevel() <= muduo::Logger::DEBUG) \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::DEBUG, __func__).stream()
#define LOG_INFO                                        \
  if (muduo::Logger::logLevel() <= muduo::Logger::INFO) \
  muduo::Logger(__FILE__, __LINE__).stream()
#define LOG_WARN muduo::Logger(__FILE__, __LINE__, muduo::Logger::WARN).stream()
#define LOG_ERROR \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::ERROR).stream()
#define LOG_FATAL \
  muduo::Logger(__FILE__, __LINE__, muduo::Logger::FATAL).stream()
#define LOG_SYSERR muduo::Logger(__FILE__, __LINE__, false).stream()
#define LOG_SYSFATAL muduo::Logger(__FILE__, __LINE__, true).stream()

// packeding strerror_r
const char* strerror_tl(int savedErrno);

// Taken from glog/logging.h
//
// Check that the input is non NULL.  This very useful in constructor
// initializer lists.

#define CHECK_NOTNULL(val)                                                 \
  ::muduo::CheckNotNull(__FILE__, __LINE__, "'" #val "' Must be non NULL", \
                        (val))

// A small helper for CHECK_NOTNULL().
template <typename T>
T* CheckNotNull(Logger::SourceFile file, int line, const char* names, T* ptr) {
  if (ptr == NULL) {
    Logger(file, line, Logger::FATAL).stream() << names;
  }
  return ptr;
}

}  // namespace muduo

#endif  // MUDUO_BASE_LOGGING_H
