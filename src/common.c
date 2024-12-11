#include <malloc.h>
#include "internal.h"

extern u8* content;     // current content

// memory utilities
// handle is a pointer (morally a reference) to a memory pointer
// *handle is assumed to equal (1) a pointer to allocated memory or (2) NULL
static inline void memReallocAlign(void** handle, size_t size) {
    free(*handle); *handle = memalign(32, (size+31)&(~31)); // (size+31)&(~31) rounds size up to multiple of 32
}
void memFree(void** handle) {
    free(*handle); *handle = NULL;
}

// returns filesize (on success), ES errors (https://www.wiibrew.org/wiki//dev/es) or own ERROR_OUTOFMEMORY
// does not require patched IOS or ISFS_Initialize
int parseTmd(u64 tid, signed_blob** handle) {
    u32 tmdSize = 0;
    s32 ret = ES_GetStoredTMDSize(tid, &tmdSize);
    if (ret == 0) {
        memReallocAlign((void**)handle, tmdSize);
        // printf("  [realloc for tid %016llx: %p (%x)]\n", tid, *handle, tmdSize);
        if (*handle != NULL) {
            ret = ES_GetStoredTMD(tid, *handle, tmdSize);
            if (ret == 0) {
                ret = tmdSize;                  // return filesize
            } else {memFree((void**)handle);}
        } else {ret = ERROR_OUTOFMEMORY;}
    }
    return ret;
}
