#include "c.h"
#include "wotr.h"
#include <string>
#include <system_error>
#include <string.h>

extern "C" {
  struct wotr_t      { Wotr* rep; };
  struct offset_t    { logoffset_t* rep; };

  wotr_t* wotr_open(const char* logfile, char** errptr) {
    Wotr* w;
    try {
      w = new Wotr(logfile);
    } catch (const std::system_error& e) {
      *errptr = strdup(e.what());
      return nullptr;
    }

    wotr_t* res = new wotr_t;
    res->rep = w;
    return res;
  }

  int wotr_register(wotr_t* w, const char* pathstr, size_t len) {
    std::string path(pathstr, len);
    return w->rep->Register(path);
  }

  void wotr_unregister(wotr_t* w, int ident) {
    return w->rep->UnRegister(ident);
  }

  int wotr_numregistered(wotr_t* w) {
    return w->rep->NumRegistered();
  }
    
  offset_t* wotr_write(wotr_t* w, const char* logdata, size_t len, int flush) {
    std::string data(logdata, len);
    offset_t* res = new offset_t;
    res->rep = w->rep->WotrWrite(data, flush);
    return res;
  }
  
  int wotr_get(wotr_t* w, size_t offset, char** data, size_t* len,
               size_t version) {
    return w->rep->WotrGet(offset, data, len, version);
  }

  void wotr_close(wotr_t* w) {
    delete w->rep;
    delete w;
  }

  int wotr_flush(wotr_t* w) {
    return w->rep->Flush();
  }
}
