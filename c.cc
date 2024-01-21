#include "c.h"
#include "wotr.h"
#include <string>
#include <system_error>
#include <string.h>

extern "C" {
  struct wotr_t      { Wotr* rep; };

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

  ssize_t wotr_write(wotr_t* w, const char* logdata, size_t len) {
    std::string data(logdata, len);
    return = w->rep->WotrWrite(data);
  }
  
  int wotr_get(wotr_t* w, size_t offset, char** data, size_t* len) {
    return w->rep->WotrGet(offset, data, len, version);
  }

  int wotr_p_get(wotr_t* w, size_t offset, char** data, size_t len) {
    return w->rep->WotrPGet(offset, data, len);
  }

  ssize_t wotr_head(wotr_t* w) {
    return w->rep->WotrHead();
  }


  int wotr_sync(wotr_t* w) {
    return w->rep->Sync();
  }

  void wotr_close(wotr_t* w) {
    delete w->rep;
    delete w;
  }
}
