#include "systitver.h"
#include "internal.h"

static u8* content = NULL;          // last loaded content file contents
static u8* shared1Map = NULL;       // retained shared content map
static int shared1MapSize = -1;     // size of above

// == shared content (internal) ==

// load shared content map file to memory
static inline int loadShared1() {
    shared1MapSize = allocReadFile("/shared1/content.map", &shared1Map);
    // shared1Map[58] = 'c'; // to test id/hash fail
	// printf(" [content.map: (%d)]", shared1MapSize);
    return shared1MapSize;
}

// look up hash in shared content map, return shared content ID (u8) or -1 if not found
static int hashToSid(u8* hash) {
    s32 ret = 0;
    if (shared1Map == NULL) { ret = loadShared1(); }
    if (ret < 0) { return ret; }

    shared1Content* s1map = (shared1Content*)shared1Map;
    bool found = false;
    for (int j=0; j<shared1MapSize/sizeof(shared1Content); j++) {
        if (memcmp(hash, s1map[j].hash, 20) == 0) {
            int sid = parseHex(s1map[j].id);
            return sid >= 0x0 && sid < 0x100 ? sid : ERROR_CONTENTMAPCORRUPTED;
        }
    }
    return -1;
}

// == shared content (api) ==

int STV_VerifyShared1(u32 mask[4], bool log) {        
    s32 ret = 0;
    if (shared1Map == NULL) { ret = loadShared1(); }
    if (ret < 0) { return ret; }
    
    static u32 shared1verified[4];  // caches already-verified shared content IDs
    static char filepath[48] = "/shared1/xxxxxxxx.app";
    static u8 hashActual[20] ATTRIBUTE_ALIGN(64);
    shared1Content* s1map = (shared1Content*)shared1Map;
    if (log) {printf("- shr cnt:");}
    for (int i=0; i<shared1MapSize/sizeof(shared1Content); i++) {
        int sid = parseHex(s1map[i].id);
        if (sid < 0x0 || sid >= 0x100) { return ERROR_CONTENTMAPCORRUPTED; }
        if (!(mask[sid/32] & (1 << (sid%32)))) { continue; }            // skip SID not in input mask
        if (shared1verified[sid/32] & (1 << (sid%32)) ) {               // skip already-verified SID
            if (log) { printf(CON_YELLOW(" (%d)"), sid); }
            continue;
        }

        for (int j=0; j<8; j++) { filepath[9+j] = s1map[i].id[j]; }
        int filesize = allocReadFile(filepath, &content);
        if (filesize < 0) { return filesize; }

        ret = iosSha(content, filesize, hashActual);
        if (ret < 0) {return ret;}
        if (memcmp(s1map[i].hash, hashActual, 20) == 0) {
            shared1verified[sid/32] |= 1 << (sid%32);
            if (log) {printf(" (%d)", sid);}
        } else {
            return -500 - sid;
        }
    }
    if (log) {printf("\n");}
    return 0;
}

// == title (api) ==

void STV_FreeContent() {
    memFree((void**)&shared1Map);
    memFree((void**)&content);
}

extern signed_blob* tmdRaw;
extern tmd* tmdParsed;
extern u32 titleIdLower;
int STV_VerifyCurrentTitle(bool log) {
    if (tmdRaw == NULL) { return ERROR_UNINITIALISED; }
    int ret = 0;
    static char filepath[48];
    u32 shared1Mask[4] = {};
    if (log) {printf("- unq cnt:");}

    for (int i=0; i<tmdParsed->num_contents; i++) {
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
            static u8 hash[20] ATTRIBUTE_ALIGN(64);
            ret = iosSha(content, filesize, hash);
            if (ret < 0) {return ret;}
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
