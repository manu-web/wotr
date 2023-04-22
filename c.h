#ifndef C_WOTR_INCLUDE_CWRAPPER_H_
#define C_WOTR_INCLUDE_CWRAPPER_H_

#pragma once
#define WOTR_LIBRARY_API

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

typedef struct wotr_t wotr_t;
typedef struct offset_t offset_t;

extern WOTR_LIBRARY_API
wotr_t* wotr_open(const char* logfile, char** errptr);
    
extern WOTR_LIBRARY_API
int wotr_register(wotr_t* w, const char* pathstr, size_t len);
    
extern WOTR_LIBRARY_API
void wotr_unregister(wotr_t* w, int ident);
    
extern WOTR_LIBRARY_API
int wotr_numregister(wotr_t* w);
    
extern WOTR_LIBRARY_API
offset_t* wotr_write(wotr_t* w, const char* logdata, size_t len);
    
extern WOTR_LIBRARY_API
int wotr_get(wotr_t* w, size_t offset, char** data, size_t* len, size_t version);

extern WOTR_LIBRARY_API
int wotr_p_get(wotr_t* w, size_t offset, size_t len, char** data);

extern WOTR_LIBRARY_API
void wotr_close(wotr_t* w);
    
extern WOTR_LIBRARY_API
int wotr_sync(wotr_t* w);


#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* C_WOTR_INCLUDE_CWRAPPER_H_ */
