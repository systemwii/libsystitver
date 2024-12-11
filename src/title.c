#include "systitver.h"
#include "internal.h"
#include "identify.h"

signed_blob* tmdRaw = NULL;         // currently loaded title TMD (full signed blob)
tmd* tmdParsed = NULL;              // TMD object pointer within above data (= above+0x140)
u32 titleIdLower = 0;               // ID of last loaded title
static u8 tmdHash[20];              // hash of last loaded TMD
static u8 cmdHash[20];              // hash of last loaded TMD's concatenated content metadata

tmd* const STV_GetCurrentTMD() {
    return tmdParsed;
}

void STV_FreeTitle() {
    memFree((void**)&tmdRaw); tmdParsed = NULL;
}

int STV_LoadTitle(u32 title, bool log) {
    // load tmd
    int filesize = parseTmd(0x0000000100000000ULL | title, &tmdRaw);
    if (filesize == -106)  { return ERROR_TITLENOTFOUND; }
    else if (filesize < 0) { return filesize; }
    // update tmd state variables
    tmdParsed = (tmd*)SIGNATURE_PAYLOAD(tmdRaw);
    if (0x1e4 > filesize || 0x1e4 + tmdParsed->num_contents*sizeof(tmd_content) > filesize) {
        STV_FreeTitle(); return ERROR_TMDCORRUPTED;
    }
    titleIdLower = title;
    s32 contentCount = tmdParsed->num_contents;
    SHA1((u8*)tmdRaw, filesize, tmdHash);
    SHA1((u8*)(tmdParsed->contents), contentCount*sizeof(tmd_content), cmdHash);
	if (log) {
        printf(CON_YELLOW("\nslot %3d | #%2d |"), titleIdLower, contentCount);
        SHA1PRINTF(tmdHash); printf("\n");
    }
    return 0;
}

// title identification heuristics

SysTitTag STV_IdentifyCurrentTitle(bool log) {
    // by tmd hash
    static int cachedCounter;   // optimisation: loop starting from where we left off last time
    for (int i=0; i<sizeof(tmdHashDict)/sizeof(SysTitTag); i++) {
        int j = (i + cachedCounter) % (sizeof(tmdHashDict)/sizeof(SysTitTag));
        if (memcmp(tmdHash, tmdHashDict[j].hash, 20) == 0) {
            if (log) {
                printf(CON_CYAN("> identified: IOS%d v%d | %s | [by tmd hash]\n"),
                    tmdHashDict[j].titleID, tmdHashDict[j].rev, tmdHashDict[j].name);
            }
            cachedCounter = j+1;
            return tmdHashDict[j];
        }
    }
    // by cmd hash
    for (int i=0; i<sizeof(cmdHashDict)/sizeof(SysTitTag); i++) {
        if (memcmp(cmdHash, cmdHashDict[i].hash, 20) == 0) {
            if (log) {
                printf(CON_CYAN("> identified: IOS%d v%d | %s | [by cmd hash]\n"),
                    cmdHashDict[i].titleID, cmdHashDict[i].rev, cmdHashDict[i].name);
            }
            return cmdHashDict[i];
        }
    }
    // not found
    return (SysTitTag){0, titleIdLower, 0, "[unknown]", tmdHash};
}
