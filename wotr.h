#ifndef WOTR_H
#define WOTR_H

#include <cstdint>
#include <string>
#include <mutex>
#include <unordered_map>
#include <fstream>

#include "rocksdb/db.h"

#define STAT_BUF_SIZE (4096)

typedef struct {
  size_t ksize;
  size_t vsize;
  uint32_t cfid;
} item_header;

struct kv_entry_info {
  size_t ksize;
  size_t vsize;
  size_t key_offset;
  size_t value_offset;
  size_t size; // entry size. use to get offset of next entry if iterating log
};

class Wotr {
public:
  Wotr (const char* logname);

  ssize_t WotrWrite(std::string& logdata);
  int WotrGet(size_t offset, char** data, size_t* len);
  int WotrPGet(size_t offset, char** data, size_t len);
  ssize_t Head();
  int Sync();

  int Register(std::string path);
  void UnRegister(std::string path);
  int NumRegistered();
  int StartupRecovery(std::string path, size_t logstart);
  int get_entry(size_t offset, struct kv_entry_info* entry);
  int CloseAndDestroy();

private:
  std::string _logname;
  int _log; // fd
  int _db_counter;
  ssize_t _offset;
  std::mutex _lock;

  std::unordered_map<std::string, rocksdb::DB*> _dbs;


  int safe_write(int fd, const char* data, size_t size);

  // maybe useful later... these are set up in Wotr::Wotr()
  int _statslog; // fd
  char* _statsstart;
  char* _statsptr;
  size_t _inception; // time in ns start of program
};

#endif // WOTR_H
