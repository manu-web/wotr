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
  _seq = 0;
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

int Wotr::Register(std::string path) {
  int ret = _db_counter;
  _dbs.insert(std::make_pair(_db_counter, path));
  _db_counter++;

  return ret;
}

void Wotr::UnRegister(int ident) {
  _dbs.erase(ident);
}

int Wotr::NumRegistered() {
  return _dbs.size();
}

int safe_write(int fd, const char* data, size_t size) {
  const char* src = data;
  size_t left = size;

  while (left != 0) {
    size_t bytes_to_write = left;

    ssize_t done = write(fd, src, bytes_to_write);
    if (done < 0) {
      if (errno == EINTR) { // rocksdb does this check, so I will do
        continue;
      }
      return -1;
    }
    left -= done;
    src += done;
  }
  return 0;
}

// append to log
logoffset_t* Wotr::WotrWrite(std::string& logdata) {
//  struct timespec ts;
//  clock_gettime(CLOCK_MONOTONIC, &ts);
//  size_t starttime = static_cast<size_t>(ts.tv_sec) * 1000000000 + ts.tv_nsec;
  if (lseek(_log, _offset, SEEK_SET) < 0) {
    std::cout << "wotrwrite: Error seeking log" << std::endl;
    return nullptr;
  }
  size_t bytes_to_write = logdata.size();

  if (safe_write(_log, logdata.data(), bytes_to_write) < 0) {
    std::cout << "wotrwrite write data: " << strerror(errno) << std::endl;
    return nullptr;
  }
  
//  clock_gettime(CLOCK_MONOTONIC, &ts);
//  size_t endtime = static_cast<size_t>(ts.tv_sec) * 1000000000 + ts.tv_nsec;

  // int len = snprintf(_statsptr, STAT_BUF_SIZE - (_statsptr - _statsstart),
  //                    "written: %ld start: %ld dur: %ld\n",
  //                    bytes_to_write, starttime - _inception,
  //                    endtime - starttime);
//  _statsptr += len;

  // flush the buffer, reset
  // if ((STAT_BUF_SIZE - (_statsptr - _statsstart)) < 100) {
  //   dprintf(_statslog, "%s", _statsstart);
  //   _statsptr = _statsstart;
  // }

  logoffset_t* ret = new logoffset_t;
  ret->offset = (size_t)_offset;
  ret->seq = _seq;
  
  _offset += logdata.size();
  return ret;
}

int Wotr::WotrGet(size_t offset, char** data, size_t* len, size_t version) {
  // if version = 0, any offset is accepted
  if (version != 0 && offset >= (size_t)(_versions[version])) {
    std::cout << "bad version number" << std::endl;
    return -1;
  }
  
  item_header header;

  // read the header
  // use pread for thread safety
  if (pread(_log, (char*)&header, sizeof(item_header), (off_t)offset) < 0) {
    std::cout << "bad read header: " << strerror(errno) << std::endl;
    return -1;
  }

  // don't need the CF id right now, but it is in the header
  char *kbuf = (char*)malloc(header.ksize * sizeof(char));
  char *vbuf = (char*)malloc(header.vsize * sizeof(char));

  off_t key_offset = (off_t)offset + sizeof(item_header);
  off_t value_offset = key_offset + header.ksize;

  if (pread(_log, kbuf, header.ksize, key_offset) < 0) {
    std::cout << "wotrget read key: " << strerror(errno) << std::endl;
    return -1;
  }

  if (pread(_log, vbuf, header.vsize, value_offset) < 0) {
    std::cout << "wotrget read value: " << strerror(errno) << std::endl;
    return -1;
  }

  *data = vbuf;
  *len = header.vsize;
  free(kbuf);

  return 0;
}

int Wotr::WotrPGet(size_t offset, size_t len, char** data) {
  char *vbuf = (char*)malloc(len * sizeof(char));
  if (pread(_log, vbuf, len, offset) < 0) {
    std::cout << "wotrpget read value: " << strerror(errno) << std::endl;
    return -1;
  }
  *data = vbuf;
  return 0;
}

int Wotr::Sync() {
  return fsync(_log);
}

int Wotr::CloseAndDestroy() {
  // flush the buffer
//  dprintf(_statslog, "%s", _statsstart);

  close(_log);
  return unlink(_logname.c_str());
}
