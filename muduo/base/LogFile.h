/*
 * @Author: Mengsen.Wang
 * @Date: 2020-05-15 18:34:30
 * @Last Modified by: Mengsen.Wang
 * @Last Modified time: 2020-05-15 19:49:52
 * @Description: log file manger
 */

// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef MUDUO_BASE_LOGFILE_H
#define MUDUO_BASE_LOGFILE_H

#include <memory>

#include "muduo/base/Mutex.h"
#include "muduo/base/Types.h"

namespace muduo {

namespace FileUtil {
class AppendFile;
}

class LogFile : noncopyable {
 public:
  LogFile(const string& basename, off_t rollSize, bool threadSafe = true,
          int flushInterval = 3, int checkEveryN = 1024);
  ~LogFile();

  // lock version append
  void append(const char* logline, int len);
  void flush();
  bool rollFile();

 private:
  // unlock version append
  void append_unlocked(const char* logline, int len);

  // get log file name
  static string getLogFileName(const string& basename, time_t* now);

  const string basename_;    // log file name
  const off_t rollSize_;     // roll size
  const int flushInterval_;  // flush interval
  const int checkEveryN_;    // check per n

  int count_;

  std::unique_ptr<MutexLock> mutex_;
  time_t startOfPeriod_;  // start time of this period
  time_t lastRoll_;       // last time of roll
  time_t lastFlush_;      // last time of flush

  std::unique_ptr<FileUtil::AppendFile> file_;  // file

  const static int kRollPerSeconds_ = 60 * 60 * 24;  // roll seconds
};

}  // namespace muduo
#endif  // MUDUO_BASE_LOGFILE_H
