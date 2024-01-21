#include <iostream>
#include <vector>
#include <system_error>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "wotr.h"

Wotr::Wotr(const char* logname) {
  _logname = std::string(logname);
  _offset = 0;
  _db_counter = 0;

  // open wotr valuelog
  if ((_log = open(logname, O_RDWR | O_CREAT | O_APPEND, S_IRWXU)) < 0) {
    std::cout << "Error opening wotr" << std::endl;
    throw std::system_error(errno, std::generic_category(), logname);
  }
  lseek(_log, 0, SEEK_END);

  std::string stats_fname = _logname + ".stats";
  std::cout << stats_fname << std::endl;
  // open stats logging file
  if ((_statslog = open(stats_fname.c_str(), O_RDWR | O_CREAT | O_APPEND, S_IRWXU)) < 0) {
    std::cout << "Error opening stats file" << std::endl;
    throw std::system_error(errno, std::generic_category(), logname);
  }

  _statsstart = (char*)malloc(STAT_BUF_SIZE * sizeof(char));
  _statsptr = _statsstart;

  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  _inception = static_cast<size_t>(ts.tv_sec) * 1000000000 + ts.tv_nsec;
}

int Wotr::get_entry(size_t offset, struct kv_entry_info* entry) {
  struct stat stbuf;
  // do we have to stat every time?
  if (fstat(_log, &stbuf) < 0) {
    std::cout << "fstat problem: " << strerror(errno) << std::endl;
    return -1;
  }

  // end of log reached
  if (offset >= stbuf.st_size) {
    return -1;
  }

  // read the header
  // use pread for thread safety
  item_header header;
  if (pread(_log, (char*)&header, sizeof(item_header), (ssize_t)offset) < 0) {
    std::cout << "get_recovery_entry: bad read header: "
	      << strerror(errno) << std::endl;
    return -1;
  }

  entry->ksize = header.ksize;
  entry->vsize = header.vsize;
  entry->key_offset = offset + sizeof(item_header);
  entry->value_offset = entry->key_offset + header.ksize;
  entry->size = sizeof(item_header) + header.ksize + header.vsize;
  
  return 0;
}

int Wotr::safe_write(int fd, const char* data, size_t size) {
  const char* src = data;
  size_t ret, remaining;
  remaining = size;

  while (remaining != 0) {
    ret = write(fd, src, remaining);
    if (ret < 0) {
      if (errno == EINTR) { continue; } // rocksdb checks this, so I will do
      return -1;
    }
    remaining -= ret;
    src += ret;
  }
  return 0;
}

// append to log
ssize_t Wotr::WotrWrite(std::string& logdata) {
  std::lock_guard<std::mutex> lock(_lock);
  
  if (lseek(_log, _offset, SEEK_SET) < 0) {
    std::cout << "wotrwrite: Error seeking log" << std::endl;
    return -1;
  }
  size_t bytes_to_write = logdata.size();

  if (safe_write(_log, logdata.data(), bytes_to_write) < 0) {
    std::cout << "wotrwrite write data: " << strerror(errno) << std::endl;
    return -1;
  }
  
  ssize_t ret = _offset;
  _offset += logdata.size();
  return ret;
}

int Wotr::WotrGet(size_t offset, char** data, size_t* len) {
  struct kv_entry_info entry;
  if (get_entry(offset, &entry) != 0) {
    std::cout << "wotrget: problem getting entry" << std::endl;
    return -1;
  }

  char *vbuf = (char*)malloc(entry.vsize * sizeof(char));
  
  if (pread(_log, vbuf, entry.vsize, entry.value_offset) < 0) {
    std::cout << "wotrget: read value: " << strerror(errno) << std::endl;
    return -1;
  }
  
  *data = vbuf;
  *len = entry.vsize;

  return 0;
}

int Wotr::WotrPGet(size_t offset, char** data, size_t len) {
  char *vbuf = (char*)malloc(len * sizeof(char));
  if (pread(_log, vbuf, len, (ssize_t)offset) < 0) {
    std::cout << "wotrpget read value: " << strerror(errno) << std::endl;
    return -1;
  }
  *data = vbuf;
  return 0;
}

int Wotr::Sync() {
  return fsync(_log);
}

ssize_t Wotr::Head() {
  std::lock_guard<std::mutex> lock(_lock);
  return _offset;
}

int Wotr::CloseAndDestroy() {
  fsync(_log);
  close(_log);
  return unlink(_logname.c_str());
}
