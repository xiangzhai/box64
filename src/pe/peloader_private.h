#ifndef __PELOADER_PRIVATE_H__
#define __PELOADER_PRIVATE_H__

typedef struct peheader_s {
    char* name;
    char* path;
} peheader_t;

peheader_t* ParsePeHeader32(FILE* f, const char* name, int exec);

#endif // __PELOADER_PRIVATE_H__
