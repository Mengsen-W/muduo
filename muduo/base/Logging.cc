/*
 * @Author: Mengsen.Wang
 * @Date: 2020-05-15 16:33:25
 * @Last Modified by: Mengsen.Wang
 * @Last Modified time: 2020-05-15 17:51:34
 * @Description: main logger
 */

// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/Logging.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <sstream>

#include "muduo/base/CurrentThread.h"
#include "muduo/base/TimeZone.h"
#include "muduo/base/Timestamp.h"

namespace muduo {

/*
class LoggerImpl
{
 public:
  typedef Logger::LogLevel LogLevel;
  LoggerImpl(LogLevel level, int old_errno, const char* file, int line);
  void finish();

  Timestamp time_;
  LogStream stream_;
  LogLevel level_;
  int line_;
  const char* fullname_;
  const char* basename_;
};
*/

__thread char t_errnobuf[512];
__thread char t_time[64];
__thread time_t t_lastSecond;

/**
 * @Brief: packaging strerror_r
 * @Param: int errno
 * @Return: const char*
 */
const char* strerror_tl(int savedErrno) {
  return strerror_r(savedErrno, t_errnobuf, sizeof t_errnobuf);
}

/**
 * @Brief: initialize log level
 * @Param: void
 * @Return: Logger::Loglevel
 */
Logger::LogLevel initLogLevel() {
  if (::getenv("MUDUO_LOG_TRACE"))
    return Logger::TRACE;
  else if (::getenv("MUDUO_LOG_DEBUG"))
    return Logger::DEBUG;
  else
    return Logger::INFO;
}

// define g_logLevel
Logger::LogLevel g_logLevel = initLogLevel();

// define log name array
// notice every member length equal 6
const char* LogLevelName[Logger::NUM_LOG_LEVELS] = {
    "TRACE ", "DEBUG ", "INFO  ", "WARN  ", "ERROR ", "FATAL ",
};

// helper class for known string length at compile time
class T {
 public:
  T(const char* str, unsigned len) : str_(str), len_(len) {
    assert(strlen(str) == len_);
  }

  const char* str_;
  const unsigned len_;
};

inline LogStream& operator<<(LogStream& s, T v) {
  s.append(v.str_, v.len_);
  return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v) {
  s.append(v.data_, v.size_);
  return s;
}

/**
 * @Brief: default Output function
 * @Param: const char* msg, int len
 * @Return: void(number of member)
 */
void defaultOutput(const char* msg, int len) {
  size_t n = fwrite(msg, 1, len, stdout);
  // FIXME check n
  (void)n;
}

/**
 * @Brief: default flush function
 * @Param: void
 * @Return: void
 */
void defaultFlush() { fflush(stdout); }
// inti global variable
Logger::OutputFunc g_output = defaultOutput;
Logger::FlushFunc g_flush = defaultFlush;
TimeZone g_logTimeZone;

}  // namespace muduo

using namespace muduo;

/**
 * @Brief: init impl construction
 * @Param: LogLevel level. int savedErrno, const SourceFile& file, int line
 */
Logger::Impl::Impl(LogLevel level, int savedErrno, const SourceFile& file,
                   int line)
    : time_(Timestamp::now()),
      stream_(),
      level_(level),
      line_(line),
      basename_(file) {
  formatTime();
  CurrentThread::tid();
  stream_ << T(CurrentThread::tidString(), CurrentThread::tidStringLength());
  stream_ << T(LogLevelName[level], 6);
  if (savedErrno != 0) {
    stream_ << strerror_tl(savedErrno) << " (errno=" << savedErrno << ") ";
  }
}

/**
 * @Brief: formate timeo
 * @Param: void
 * @Return: void
 */
void Logger::Impl::formatTime() {
  int64_t microSecondsSinceEpoch = time_.microSecondsSinceEpoch();
  time_t seconds = static_cast<time_t>(microSecondsSinceEpoch /
                                       Timestamp::kMicroSecondsPerSecond);
  int microseconds = static_cast<int>(microSecondsSinceEpoch %
                                      Timestamp::kMicroSecondsPerSecond);
  if (seconds != t_lastSecond) {
    t_lastSecond = seconds;
    struct tm tm_time;
    if (g_logTimeZone.valid()) {
      tm_time = g_logTimeZone.toLocalTime(seconds);
    } else {
      ::gmtime_r(&seconds, &tm_time);  // FIXME TimeZone::fromUtcTime
    }

    int len =
        snprintf(t_time, sizeof(t_time), "%4d%02d%02d %02d:%02d:%02d",
                 tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
                 tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
    assert(len == 17);
    (void)len;
  }

  if (g_logTimeZone.valid()) {
    Fmt us(".%06d ", microseconds);
    assert(us.length() == 8);
    stream_ << T(t_time, 17) << T(us.data(), 8);
  } else {
    Fmt us(".%06dZ ", microseconds);
    assert(us.length() == 9);
    stream_ << T(t_time, 17) << T(us.data(), 9);
  }
}

/**
 * @Brief: finish log
 * @Param: void
 * @Return: void
 */
void Logger::Impl::finish() {
  stream_ << " - " << basename_ << ':' << line_ << '\n';
}

Logger::Logger(SourceFile file, int line) : impl_(INFO, 0, file, line) {}

Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
    : impl_(level, 0, file, line) {
  impl_.stream_ << func << ' ';
}

Logger::Logger(SourceFile file, int line, LogLevel level)
    : impl_(level, 0, file, line) {}

Logger::Logger(SourceFile file, int line, bool toAbort)
    : impl_(toAbort ? FATAL : ERROR, errno, file, line) {}

/**
 * @Brief: every time callback will de-constrcution
 */
Logger::~Logger() {
  impl_.finish();
  const LogStream::Buffer& buf(stream().buffer());
  g_output(buf.data(), buf.length());
  if (impl_.level_ == FATAL) {
    // if FATAL abort()
    g_flush();
    abort();
  }
}

/**
 * @Brief: set log level
 * @Param: Logger::LogLevel level
 * @Return: void
 */
void Logger::setLogLevel(Logger::LogLevel level) { g_logLevel = level; }

/**
 * @Brief: set out put function
 * @Param: OutPutFunc out
 * @Return: void
 */
void Logger::setOutput(OutputFunc out) { g_output = out; }

/**
 * @Brief: set flush function
 * @Param: FlushFunc flush
 * @Return: void
 */
void Logger::setFlush(FlushFunc flush) { g_flush = flush; }

/**
 * @Brief: set Time zone
 * @Param: const TimeZone& tz
 * @Return: void
 */
void Logger::setTimeZone(const TimeZone& tz) { g_logTimeZone = tz; }
