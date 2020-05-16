/*
 * @Author: Mengsen.Wang
 * @Date: 2020-05-16 11:23:47
 * @Last Modified by:   Mengsen.Wang
 * @Last Modified time: 2020-05-16 11:23:47
 * @Description: manger all of file
 */

// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo/base/FileUtil.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "muduo/base/Logging.h"

using namespace muduo;

/**
 * @Brief: AppendFile construction
 */
FileUtil::AppendFile::AppendFile(StringArg filename)
    : fp_(::fopen(filename.c_str(), "ae")),  // 'e' for O_CLOEXEC
      writtenBytes_(0) {
  assert(fp_);
  // fopen should set buffer
  ::setbuffer(fp_, buffer_, sizeof buffer_);
  // posix_fadvise POSIX_FADV_DONTNEED ?
}

/**
 * @Brief: AppendFile destruction
 */
FileUtil::AppendFile::~AppendFile() {
  // fopen corresponds fclose
  ::fclose(fp_);
}

/**
 * @Brief: packaging AppendFile::write()
 * @Param: const char *logline, const size_t len
 * @Return: void
 */
void FileUtil::AppendFile::append(const char* logline, const size_t len) {
  // main system call
  size_t n = write(logline, len);
  size_t remain = len - n;
  while (remain > 0) {
    size_t x = write(logline + n, remain);
    if (x == 0) {
      int err = ferror(fp_);
      if (err) {
        fprintf(stderr, "AppendFile::append() failed %s\n", strerror_tl(err));
      }
      break;
    }
    // n mean write-up
    n += x;
    // remain write
    remain = len - n;  // remain -= x
  }

  // update length
  writtenBytes_ += len;
}

/**
 * @Brief: packaging fflush() system call
 * @Param: void
 * @Return: void
 */
void FileUtil::AppendFile::flush() {
  // fflush correspond fclose
  ::fflush(fp_);
}

/**
 * @Brief: Encapsulated fwrite_unlocked
 * @Param: const char* logline, size_t len
 * @Return: size_t (fwrite_unlocked() return)
 */
size_t FileUtil::AppendFile::write(const char* logline, size_t len) {
  // #undef fwrite_unlocked
  return ::fwrite_unlocked(logline, 1, len, fp_);
}

/**
 * @Brief: ReadSmallFile construction open file
 */
FileUtil::ReadSmallFile::ReadSmallFile(StringArg filename)
    : fd_(::open(filename.c_str(), O_RDONLY | O_CLOEXEC)), err_(0) {
  // open no buffer
  buf_[0] = '\0';
  if (fd_ < 0) {
    err_ = errno;
  }
}

/**
 * @Brief: destruction ReadSmallFile close file
 */
FileUtil::ReadSmallFile::~ReadSmallFile() {
  if (fd_ >= 0) {
    // open correspond close
    ::close(fd_);  // FIXME: check EINTR
  }
}

/**
 * @Brief: read file content to String content
 * @Param: int maxSize, String *content,int64_t *fileSize,
 *         int64_t* modifyTime, int64_t *createTime
 * @Return: int errno
 */
template <typename String>
int FileUtil::ReadSmallFile::readToString(int maxSize, String* content,
                                          int64_t* fileSize,
                                          int64_t* modifyTime,
                                          int64_t* createTime) {
  // compile-time assert  OFFSET
  static_assert(sizeof(off_t) == 8, "_FILE_OFFSET_BITS = 64");
  // assert content non NULL
  assert(content != NULL);
  int err = err_;

  // clear content
  if (fd_ >= 0) {
    content->clear();

    if (fileSize) {
      // get file stat
      struct stat statbuf;
      if (::fstat(fd_, &statbuf) == 0) {
        if (S_ISREG(statbuf.st_mode)) {
          // S_ISREG conventional file

          // get file size pointer
          *fileSize = statbuf.st_size;
          // reserve capacity
          content->reserve(static_cast<int>(
              std::min(implicit_cast<int64_t>(maxSize), *fileSize)));
        } else if (S_ISDIR(statbuf.st_mode)) {
          // S_ISDIR dirtory file
          err = EISDIR;
        }
        if (modifyTime) {
          // get modify Time pointer in statbuf
          *modifyTime = statbuf.st_mtime;
        }
        if (createTime) {
          // get create Time pointer in statbuf
          *createTime = statbuf.st_ctime;
        }
      } else {
        err = errno;
      }
    }

    while (content->size() < implicit_cast<size_t>(maxSize)) {
      // unread over
      // get minimun size between need to read size with buffer size
      size_t toRead = std::min(implicit_cast<size_t>(maxSize) - content->size(),
                               sizeof(buf_));
      // read system call
      ssize_t n = ::read(fd_, buf_, toRead);
      if (n > 0) {
        // actually read content and append in
        content->append(buf_, n);
      } else {
        if (n < 0) {
          err = errno;
        }
        break;
      }
    }
  }
  return err;
}

int FileUtil::ReadSmallFile::readToBuffer(int* size) {
  int err = err_;
  if (fd_ >= 0) {
    // especially useful in multi-thread
    // allow multi-thread to proform I/O on same fd
    // without being affected by changes to the file offset bt other thread
    ssize_t n = ::pread(fd_, buf_, sizeof(buf_) - 1, 0);
    if (n >= 0) {
      if (size) {
        *size = static_cast<int>(n);
      }
      buf_[n] = '\0';
    } else {
      err = errno;
    }
  }
  return err;
}

// template explicit instantiation
template int FileUtil::readFile(StringArg filename, int maxSize,
                                string* content, int64_t*, int64_t*, int64_t*);

template int FileUtil::ReadSmallFile::readToString(int maxSize, string* content,
                                                   int64_t*, int64_t*,
                                                   int64_t*);
