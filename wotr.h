#ifndef WOTR_H
#define WOTR_H

#include <string>

typedef struct {
    size_t ksize;
    size_t vsize;
} item_header;

class Wotr {
public:
  Wotr (const char* logname);

  size_t CurrentOffset();
  int WotrWrite(std::string& logdata, int flush);
  int WotrGet(size_t offset, char** data, size_t* len);
  int Flush();

private:
    std::string _logname;
    int _log; // fd 
};

#endif // WOTR_H
