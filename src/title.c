#include "systitver.h"
#include "internal.h"
#include "identify.h"

extern u8* content;
static u32 titleIdLower;            // currently loaded title
static signed_blob* tmdRaw = NULL;  // TMD of this title
static u8 tmdHash[20];              // hash of this TMD
static s32 contentCount;            // number of content files of this title

void STV_FreeTitle() {
    free(content);  content = NULL;
    free(tmdRaw);   tmdRaw = NULL;
}

// todo: this trusts the tmd data too much;
// should probably rewrite with bounds and data checks
int STV_LoadTitle(u32 title, bool log) {
    int filesize = parseTmd(0x0000000100000000ULL | title, &tmdRaw);
    if (filesize == -106)  { return ERROR_TITLENOTFOUND; }
    else if (filesize < 0) { return filesize; }

    titleIdLower = title;
    tmd* tmdParsed = (tmd*)SIGNATURE_PAYLOAD(tmdRaw);
    contentCount = tmdParsed->num_contents;
    SHA1((u8*)tmdRaw, filesize, tmdHash);
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
    if (log) {printf("- unique cnt:");}

    for (int i=0; i<contentCount; i++) {
        u32 cid = tmdParsed->contents[i].cid;
        if (tmdParsed->contents[i].type == 0x8001) {    // shared content file
            s32 sid = hashToSid(tmdParsed->contents[i].hash);
            if (sid >= 0 && sid <= 127) {
                shared1Mask[sid/32] |= 1 << (sid%32);
            } else {
                if (log) {printf(CON_MAGENTA("missing shared content:")); SHA1PRINTF(tmdParsed->contents[i].hash); printf("\n");}
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

SysTitTag STV_IdentifyCurrentTitleByHash(bool log) {
    static int cachedCounter;   // optimisation: loop starting from where we left off last time
    for (int i=0; i<sizeof(tmdHashDict)/sizeof(SysTitTag); i++) {
        int j = (i + cachedCounter) % (sizeof(tmdHashDict)/sizeof(SysTitTag));
        if (memcmp(tmdHash, tmdHashDict[j].tmdHash, 20) == 0) {
            if (log) {
                printf(CON_CYAN("> identified by tmd hash: IOS%d v%d | %s\n"),
                    tmdHashDict[j].titleID, tmdHashDict[j].rev, tmdHashDict[j].family);
            }
            cachedCounter = j+1;
            return tmdHashDict[j];
        }
    }
    return (SysTitTag){-1, 0,"","",{}};
}

SysTitTag STV_IdentifyMIOSBySize(bool log) {
    if (titleIdLower != 257) {
        int ret = STV_LoadTitle(257, log);
        if (ret != 0) { return (SysTitTag){ret, 0,"","",{}}; }
    }

    tmd* tmdParsed = (tmd*)SIGNATURE_PAYLOAD(tmdRaw);
    if (tmdParsed->num_contents < 2) { return (SysTitTag){ERROR_UNIQUECONTENTNOTFOUND, 0,"","",{}}; }

    static char filepath[48];
    snprintf(filepath, sizeof(filepath), "/title/00000001/%08x/content/%08x.app", titleIdLower, tmdParsed->contents[1].cid);
    int filesize = allocReadFile(filepath, &content);
    if (filesize == -106)   { return (SysTitTag){ERROR_UNIQUECONTENTNOTFOUND, 0,"","",{}}; }
    else if (filesize < 16) { return (SysTitTag){-400 - tmdParsed->contents[1].cid, 0,"","",{}}; }

    SysTitTag result = {};
    switch (((u32*)content)[2]) {  // = *(file+0x8); we will hope these are uniquely keyed
        case 0x000407B4:    result = (SysTitTag){257, 10, "WiiGator (WiiPower)", "", {}}; break;
        case 0x0005F3B4:    result = (SysTitTag){257,  4, "WiiGator (GCBL 0.2)", "", {}}; break;
        case 0x000545F4:    result = (SysTitTag){257,  4, "Waninkoko (rev5)",    "", {}}; break;
        case 0x0018895C:    result = (SysTitTag){257,  0, "DIOS MIOS Lite v1.3", "", {}}; break;
        case 0x0002C0D4:    result = (SysTitTag){257, 10, "official", "", {}}; break;
        case 0x0002BF50:    result = (SysTitTag){257,  9, "official", "", {}}; break;
        case 0x0002BDC8:    result = (SysTitTag){257,  8, "official", "", {}}; break;
        case 0x0002B954:    result = (SysTitTag){257,  5, "official", "", {}}; break;
        case 0x0002CB94:    result = (SysTitTag){257,  4, "official", "", {}}; break;
        default: return (SysTitTag){-1, 0,"","",{}};
    }
    if (log) {printf(CON_CYAN("> identified by size: MIOS v%d %s\n"), result.rev, result.family);}
    return result;
}
