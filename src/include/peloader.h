#ifndef __PE_LOADER_H__
#define __PE_LOADER_H__

typedef struct peheader_s peheader_t;

peheader_t* LoadAndCheckPeHeader(FILE* f, const char* name, int exec); // exec : 0 = dll, 1 = exe

#endif // __PE_LOADER_H__
