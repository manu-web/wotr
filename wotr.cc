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

int Wotr::Register(std::string path) {
  int ret = _db_counter;
  _dbs.insert(std::make_pair(_db_counter, path));
  _db_counter++;

  return ret;
}

void Wotr::UnRegister(int ident) {
  _dbs.erase(ident);
}

int StartupRecovery(std::string path, size_t location) {
  std::lock_guard<std::mutex> lock(_lock);

  struct kv_entry_info* entry;
  struct stat stbuf;
  fstat(_log, &stbuf);

  DB* db = _dbs.get(path)

  while (location < stbuf.st_size) {
    if (lseek(_log, location, SEEK_SET) < 0) {
      std::cout << "startuprecovery: Error seeking log" << std::endl;
      return -1;
    }

    entry = get_recovery_entry(location);
    if (entry == nullptr) {
      std::cout << "startuprecovery: read entry error at " << location << std::endl;
      return -1;
    }

    char* key = malloc(sizeof(char) * entry->ksize);

    if (pread(_log, &key, entry->ksize, entry->key_offset) < 0) {
      std::cout << "startup_recovery: bad read key" << std::endl;
    }

    // to_string?
    Status s = db->Put(key, std::to_string(entry->value_offset));

    if (!s.ok()) {
      std::cout << "startuprecovery: db put error" << std::endl;
      return -1;
    }
    location += entry->size;
  }
}

int Wotr::NumRegistered() {
  return _dbs.size();
}

struct kv_entry_info* get_recovery_entry(size_t offset) {
  // read the header
  // use pread for thread safety
  item_header header;
  if (pread(_log, (char*)&header, sizeof(item_header), (ssize_t)offset) < 0) {
    std::cout << "get_recovery_entry: bad read header: "
	      << strerror(errno) << std::endl;
    return nullptr;
  }

  struct kv_entry_info* entry = malloc(sizeof(struct kv_entry_info));
  if (entry == NULL) {
    return nullptr;
  }

  entry->ksize = header.ksize;
  entry->vsize = header.vsize;
  entry->key_offset = offset + sizeof(item_header);
  entry->value_offset = entry->key_offset + header.ksize;
  entry->next = sizeof(item_header) + header.ksize + header.vsize;
  
  return entry;
}

int safe_write(int fd, const char* data, size_t size) {
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
  // read the header
  // use pread for thread safety
  item_header header;
  if (pread(_log, (char*)&header, sizeof(item_header), (ssize_t)offset) < 0) {
    std::cout << "bad read header: " << strerror(errno) << std::endl;
    return -1;
  }

  // don't need the CF id right now, but it is in the header
  // don't really need the kbuf either, but it might be handy later
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
