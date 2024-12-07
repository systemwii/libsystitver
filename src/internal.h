#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gccore.h>
#include "sha1.h"

// native errors
#define ERROR_UNINITIALISED         -31     // called STV_VerifyCurrentTitle with no title loaded
#define ERROR_OUTOFMEMORY           -32     // not enuff memory to load a required file
#define ERROR_TITLENOTFOUND         -33     // title requested in STV_LoadTitle not found
#define ERROR_UNIQUECONTENTNOTFOUND -34     // verification failed: a unique content file was missing
#define ERROR_SHAREDCONTENTNOTFOUND -35     // verification failed: a shared content file was missing
#define ERROR_CONTENTMAPCORRUPTED   -36     // verification failed: failed to parse shared content ID from shared content map
#define ERROR_TMDCORRUPTED          -37     // verification failed: failed to parse content hashes from TMD
// -4xx     verification failed on the unique content file with ID xx
// -5xx     verification failed on the shared content file with ID xx
// external errors
// -5       IOS runtime patching failed because AHBPROT was not disabled (app needs to be launched from HBC with <ahb_access/> in meta.xml)
// -7       IOS runtime patching failed because librtip failed to find the right code to patch in the currently-loaded IOS
// -1xx     errors returned by IOS FS module (https://www.wiibrew.org/wiki//dev/fs)
// -10xx    errors returned by IOS ES module (https://www.wiibrew.org/wiki//dev/es)
// other?   check -10xx (ES errors) first

#define CEILING32(x)        ((x+31)&(~31))  // round up to multiple of 32 (for memalign call)
#define SHA1PRINTF(hash)    (printf(" %08x%08x%08x%08x%08x", ((u32*)hash)[0], ((u32*)hash)[1], ((u32*)hash)[2], ((u32*)hash)[3], ((u32*)hash)[4]))

typedef struct { char id[8]; u8 hash[20]; } __attribute__((packed)) shared1Content;     // content.map is just an array of these

// documentation in source files
int initNand(void);                                 // returns 0 or error
int allocReadFile(char* filepath, u8** dataPtr);    // returns + filesize or - error
int parseTmd(u64 tid, signed_blob** tmdPtr);        // returns + filesize or - error
int parseHex(char s[8]);                            // returns parsed int (-1 on error)
int loadShared1(void);                              // returns 0 or error
int hashToSid(u8* hash);                            // returns 0 or error

// text colours from monke
#define CON_RED(str)		"\x1b[31;1m" str "\x1b[37;1m"
#define CON_GREEN(str)		"\x1b[32;1m" str "\x1b[37;1m"
#define CON_YELLOW(str)		"\x1b[33;1m" str "\x1b[37;1m"
#define CON_BLUE(str)		"\x1b[34;1m" str "\x1b[37;1m"
#define CON_MAGENTA(str)	"\x1b[35;1m" str "\x1b[37;1m"
#define CON_CYAN(str)		"\x1b[36;1m" str "\x1b[37;1m"
