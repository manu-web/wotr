#include "c.h"
#include "wotr.h"
#include <string>

extern "C" {
  struct wotr_t      { Wotr* rep; };

  wotr_t* wotr_open(const char* logfile) {
    Wotr* w = new Wotr(logfile);
    wotr_t* res = new wotr_t;
    res->rep = w;
    return res;
  }
    
  int wotr_write(wotr_t* w, const char* logdata, size_t len) {
    std::string data(logdata, len);
    return w->rep->WotrWrite(data);
  }
  
  int wotr_get(wotr_t* w, size_t offset, char** data, size_t* len) {
    return w->rep->WotrGet(offset, data, len);
  } 
}
