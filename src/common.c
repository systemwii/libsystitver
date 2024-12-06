#include <malloc.h>
#include "internal.h"
#include "rtip.h"

u8* content = NULL;                 // current content
static bool nandAccess = false;     // caches whether nand access has been patched into runtime

// patch IOS for nand access and initialise file system
int initNand() {
    s32 ret = IosPatch_RUNTIME(PATCH_WII, PATCH_ISFS_PERMISSIONS, false);
    if (ret < 0) {return ret;}
    ret = ISFS_Initialize();
    if (ret < 0) {return ret;}
    nandAccess = true;
    return 0;
}

// read file to heap, set address to *dataPtr, return size or error
// for use with reusable static pointers; frees them before setting them again
// returns filesize (on success), ISFS errors (https://www.wiibrew.org/wiki//dev/fs) or own ERROR_OUTOFMEMORY
int allocReadFile(char* filepath, u8** dataPtr) {
    s32 ret = 0;
    if (!nandAccess) {ret = initNand();}
    if (ret != 0) {return ret;}

    s32 fd = ISFS_Open(filepath, ISFS_OPEN_READ);
    if (fd > 0) {
        static fstats filestat ATTRIBUTE_ALIGN(32);
        ret = ISFS_GetFileStats(fd, &filestat);
        if (ret == 0) {
            // printf(" [realloc (%d) for %s]\n", CEILING32(filestat.file_length), filepath);
            free(*dataPtr);
            *dataPtr = memalign(32, CEILING32(filestat.file_length));
            if (*dataPtr != NULL) {
                ret = ISFS_Read(fd, *dataPtr, filestat.file_length);    // returns filesize
            } else {ret = ERROR_OUTOFMEMORY;}
        }
        ISFS_Close(fd);
    } else {ret = fd;}
    return ret;
}

// returns filesize (on success), ES errors (https://www.wiibrew.org/wiki//dev/es) or own ERROR_OUTOFMEMORY
int parseTmd(u64 tid, signed_blob** tmdPtr) {
    s32 ret = 0;
    if (!nandAccess) {ret = initNand();}
    if (ret != 0) {return ret;}

    u32 tmdSize = 0;
    ret = ES_GetStoredTMDSize(tid, &tmdSize);
    if (ret == 0) {
        // printf(" [realloc (%d) for tid %llu]\n", CEILING32(tmdSize), tid);
        free(*tmdPtr);
        *tmdPtr = memalign(32, CEILING32(tmdSize));
        if (*tmdPtr != NULL) {
            memset(*tmdPtr, 0, CEILING32(tmdSize));
            ret = ES_GetStoredTMD(tid, *tmdPtr, tmdSize);
            if (ret == 0) {ret = tmdSize;}      // returns filesize
        } else {ret = ERROR_OUTOFMEMORY;}
    }
    return ret;
}

// convert lowercase hex string to integer, -1 if invalid (or 0xffffffff)
int parseHex(char s[8]) {
    int result = 0;
    for (int i=0; i<8; i++) {
        char c = s[7-i];
        int val = 0;
        if      (c >= '0' && c <= '9') { val = c - '0';      }
        else if (c >= 'a' && c <= 'f') { val = c - 'a' + 10; }
        else    { return -1; }
        result += val * (1 << (i*4));
    }
    return result;
}
