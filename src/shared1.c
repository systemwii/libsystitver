#include "systitver.h"
#include "internal.h"

extern u8* content;
static u8* shared1Map = NULL;       // retained shared content map
static int shared1MapSize = -1;     // size of above

// load shared content map file to memory
int loadShared1() {
    shared1MapSize = allocReadFile("/shared1/content.map", &shared1Map);
    // shared1Map[58] = 'c'; // to test id/hash fail
	// printf(" [content.map: (%d)]", shared1MapSize);
    return shared1MapSize;
}

// look up hash in shared content map, return shared content ID (u8) or -1 if not found
int hashToSid(u8* hash) {
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

void STV_FreeShared1() {
    memFree((void**)&shared1Map);
}

int STV_VerifyShared1(u32 mask[4], bool log) {        
    s32 ret = 0;
    if (shared1Map == NULL) { ret = loadShared1(); }
    if (ret < 0) { return ret; }
    
    static u32 shared1verified[4];  // caches already-verified shared content IDs
    static char filepath[48] = "/shared1/xxxxxxxx.app";
    u8 hashActual[20];
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

        SHA1(content, filesize, hashActual);
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
