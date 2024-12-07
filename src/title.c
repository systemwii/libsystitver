#include "systitver.h"
#include "internal.h"
#include "identify.h"

extern u8* content;
static signed_blob* tmdRaw = NULL;  // currently loaded title TMD
static u32 titleIdLower;            // ID of this title
static s32 contentCount;            // number of content files of this title
static u8 tmdHash[20];              // hash of this TMD
static u8 cmdHash[20];              // hash of this TMD's concatenated content metadata

void STV_FreeTitle() {
    memFree((void**)&content);
    memFree((void**)&tmdRaw);
}

int STV_LoadTitle(u32 title, bool log) {
    // load tmd
    int filesize = parseTmd(0x0000000100000000ULL | title, &tmdRaw);
    if (filesize == -106)  { return ERROR_TITLENOTFOUND; }
    else if (filesize < 0) { return filesize; }
    // parse tmd
    tmd* tmdParsed = (tmd*)SIGNATURE_PAYLOAD(tmdRaw);
    if (0x1e4 + tmdParsed->num_contents*sizeof(tmd_content) > filesize) {
        STV_FreeTitle(); return ERROR_TMDCORRUPTED;
    }
    // success: update tmd state variables
    titleIdLower = title;
    contentCount = tmdParsed->num_contents;
    SHA1((u8*)tmdRaw, filesize, tmdHash);
    SHA1((u8*)(tmdParsed->contents), contentCount*sizeof(tmd_content), cmdHash);
	if (log) {
        printf(CON_YELLOW("\nslot %3d | #%2d |"), titleIdLower, contentCount);
        SHA1PRINTF(tmdHash); printf("\n");
    }
    return 0;
}

int STV_VerifyCurrentTitle(bool log) {
    if (tmdRaw == NULL) { return ERROR_UNINITIALISED; }
    int ret = 0;
    static char filepath[48];
    u32 shared1Mask[4] = {};
    tmd* tmdParsed = (tmd*)SIGNATURE_PAYLOAD(tmdRaw);
    if (log) {printf("- unq cnt:");}

    for (int i=0; i<contentCount; i++) {
        u32 cid = tmdParsed->contents[i].cid;
        if (tmdParsed->contents[i].type == 0x8001) {    // shared content file
            s32 sid = hashToSid(tmdParsed->contents[i].hash);
            if (sid >= 0 && sid <= 127) {
                shared1Mask[sid/32] |= 1 << (sid%32);
            } else {
                if (log) {
                    printf(CON_MAGENTA("missing shared content:"));
                    SHA1PRINTF(tmdParsed->contents[i].hash); printf("\n");
                }
                return ERROR_SHAREDCONTENTNOTFOUND;
            }
        } else {    // unique content file
            snprintf(filepath, sizeof(filepath), "/title/00000001/%08x/content/%08x.app", titleIdLower, cid);
            int filesize = allocReadFile(filepath, &content);
            if (filesize == -106) {
                if (log) {printf(CON_MAGENTA("missing unique content: %s\n"), filepath);}
                return ERROR_UNIQUECONTENTNOTFOUND;
            }
            else if (filesize < 0) { return filesize; }
            u8 hash[20];
            SHA1(content, filesize, hash);
            ret = memcmp(tmdParsed->contents[i].hash, hash, 20);
            if (ret == 0) {
                if (log) {printf(" <%d>", cid);}
            } else {
                return -400 - cid;
            }
        }
    }
    if (log) {printf("\n");}

    ret = STV_VerifyShared1(shared1Mask, log);
    if (ret == 0) {
        if (log) {printf(CON_GREEN("- slot %3d passed verification\n"), titleIdLower);}
    }
    return ret;
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
