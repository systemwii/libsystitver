#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gccore.h>

// native errors
#define ERROR_UNINITIALISED         -31     // called STV_VerifyCurrentTitle with no title loaded
#define ERROR_OUTOFMEMORY           -32     // not enuff memory to load a required file
#define ERROR_TITLENOTFOUND         -33     // title requested in STV_LoadTitle not found
#define ERROR_TMDCORRUPTED          -37     // verification failed: failed to parse content hashes from TMD
// external errors
// -5       IOS runtime patching failed because AHBPROT was not disabled (app needs to be launched from HBC with <ahb_access/> in meta.xml)
// -7       IOS runtime patching failed because librtip failed to find the right code to patch in the currently-loaded IOS
// -1xx     errors returned by IOS FS module (https://www.wiibrew.org/wiki//dev/fs)
// -10xx    errors returned by IOS ES module (https://www.wiibrew.org/wiki//dev/es)
// other?   check -10xx (ES errors) first

#define SHA1PRINTF(hash)    (printf(" %08x%08x%08x%08x%08x", ((u32*)hash)[0], ((u32*)hash)[1], ((u32*)hash)[2], ((u32*)hash)[3], ((u32*)hash)[4]))

// documentation in source files
void memFree(void** handle);
int iosSha(u8* data, u32 size, u8* output);
int parseTmd(u64 tid, signed_blob** tmdPtr);        // returns + filesize or - error

// text colours from monke
#define CON_RED(str)		"\x1b[31;1m" str "\x1b[37;1m"
#define CON_GREEN(str)		"\x1b[32;1m" str "\x1b[37;1m"
#define CON_YELLOW(str)		"\x1b[33;1m" str "\x1b[37;1m"
#define CON_BLUE(str)		"\x1b[34;1m" str "\x1b[37;1m"
#define CON_MAGENTA(str)	"\x1b[35;1m" str "\x1b[37;1m"
#define CON_CYAN(str)		"\x1b[36;1m" str "\x1b[37;1m"
