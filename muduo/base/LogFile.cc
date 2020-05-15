// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/LogFile.h"

#include <assert.h>
#include <stdio.h>
#include <time.h>

#include "muduo/base/FileUtil.h"
#include "muduo/base/ProcessInfo.h"

using namespace muduo;

/**
 * @Brief: constructor
 */
LogFile::LogFile(const string& basename, off_t rollSize, bool threadSafe,
                 int flushInterval, int checkEveryN)
    : basename_(basename),
      rollSize_(rollSize),
      flushInterval_(flushInterval),
      checkEveryN_(checkEveryN),
      count_(0),
      mutex_(threadSafe ? new MutexLock : NULL),
      startOfPeriod_(0),
      lastRoll_(0),
      lastFlush_(0) {
  // assert find '/'
  assert(basename.find('/') == string::npos);
  rollFile();
}

LogFile::~LogFile() = default;

/**
 * @Brief: backing append_unlocked
 * @Param: const char* logline, int len
 * @Return: void
 */
void LogFile::append(const char* logline, int len) {
  if (mutex_) {
    MutexLockGuard lock(*mutex_);
    append_unlocked(logline, len);
  } else {
    append_unlocked(logline, len);
  }
}

/**
 * @Brief: flush the buffer
 * @Param: void
 * @Return: void
 */
void LogFile::flush() {
  if (mutex_) {
    MutexLockGuard lock(*mutex_);
    file_->flush();
  } else {
    file_->flush();
  }
}

/**
 * @Brief: unlocked append
 * @Param: const char* logline, int len
 * @Return: void
 */
void LogFile::append_unlocked(const char* logline, int len) {
  // append to file
  file_->append(logline, len);

  if (file_->writtenBytes() > rollSize_) {
    // file write enough to rollfile
    rollFile();
  } else {
    // plus count
    ++count_;
    if (count_ >= checkEveryN_) {
      // count >= check every N
      count_ = 0;
      time_t now = ::time(NULL);
      time_t thisPeriod_ = now / kRollPerSeconds_ * kRollPerSeconds_;
      if (thisPeriod_ != startOfPeriod_) {
        // maybe enough time period
        rollFile();
      } else if (now - lastFlush_ > flushInterval_) {
        // enough time th flush
        lastFlush_ = now;
        file_->flush();
      }
    }
  }
}

/**
 * @Brief: return bool for rollfile successful
 * @Param: void
 * @Return: bool
 */
bool LogFile::rollFile() {
  time_t now = 0;
  string filename = getLogFileName(basename_, &now);
  time_t start = now / kRollPerSeconds_ * kRollPerSeconds_;

  if (now > lastRoll_) {
    // condition of satisfaction
    lastRoll_ = now;
    lastFlush_ = now;
    startOfPeriod_ = start;
    file_.reset(new FileUtil::AppendFile(filename));
    return true;
  }
  return false;
}

/**
 * @Brief: get log file name and time
 * @Param: const string& basename, time_t* now(return)
 * @Return: string
 */
string LogFile::getLogFileName(const string& basename, time_t* now) {
  string filename;
  filename.reserve(basename.size() + 64);
  filename = basename;

  char timebuf[32];
  struct tm tm;
  *now = time(NULL);
  gmtime_r(now, &tm);  // FIXME: localtime_r ?
  strftime(timebuf, sizeof timebuf, ".%Y%m%d-%H%M%S.", &tm);
  filename += timebuf;

  filename += ProcessInfo::hostname();

  char pidbuf[32];
  snprintf(pidbuf, sizeof pidbuf, ".%d", ProcessInfo::pid());
  filename += pidbuf;

  filename += ".log";

  return filename;
}
