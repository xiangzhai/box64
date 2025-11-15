#include <stdio.h>

#include "peloader_private.h"

peheader_t* LoadAndCheckPeHeader(FILE* f, const char* name, int exec)
{
    peheader_t* h = ParsePeHeader32(f, name, exec);
    return h;
}
