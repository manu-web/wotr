#ifndef WOTR_H
#define WOTR_H

#include <cstdint>
#include <string>
#include <mutex>
#include <unordered_map>
#include <fstream>

typedef struct {
  size_t ksize;
  size_t vsize;
  uint32_t cfid;
} item_header;

typedef struct {
  size_t seq;
  size_t offset;
} logoffset_t;

class Wotr {
public:
  Wotr (const char* logname);

  logoffset_t* WotrWrite(std::string& logdata, int flush);
  int WotrGet(size_t offset, char** data, size_t* len, size_t version);
  int WotrPGet(size_t offset, size_t len, char** data);
  int Flush();

  int Register(std::string path);
  void UnRegister(int ident);
  int NumRegistered();
  int CloseAndDestroy();

private:
  std::string _logname;
  int _log; // fd
  int _db_counter;
  size_t _seq;
  off_t _offset;
  std::ofstream _statslog;
  std::unordered_map<int, std::string> _dbs;
  std::unordered_map<size_t, off_t> _versions;
};

#endif // WOTR_H
