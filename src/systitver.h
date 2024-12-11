#pragma once
#pragma GCC visibility push(default)
#include <gccore.h>

// required reading: readme.md ("Wii System Title Verification In Brief")


// == API ==
// the log parameters below enables printf logging (set up a console to read it)

typedef struct {
    s32 type;           // 0 (no match); 1 (tmd hash), 2 (cmd hash)
    s16 titleID;        // the title slot it's installed to (= IOS number) // negative on error
    u16 rev;            // title revision
    char* name;         // ios name ("original" for official titles)
    u8* hash;           // sha1 hash of title metadata
} SysTitTag;

int STV_LoadTitle(u32 titleIdLower, bool log);  // loads title #titleIdLower to memory
tmd* const STV_GetCurrentTMD(void);             // provides access to currently-loaded title's TMD (w/o signature)
SysTitTag STV_IdentifyCurrentTitle(bool log);   // identify title by hash of full TMD or TMD content block
// you can add your own title identification heuristics, prefixed "SysTitTag STV_Identify"
// discussion of their pros and cons is at identify.h

int STV_VerifyCurrentTitle(bool log);           // verifies all content files match the hashes in the TMD
int STV_VerifyShared1(u32 mask[4], bool log);   // verifies shared content files specified by bitmask mask
// the mask comprises shared content file IDs (SIDs) as specified in content.map (maximum supported SID: 127)
// so for example, to verify SIDs 5 and 57, do
//     u32 mask[4] = {};
//     mask[ 5/32] |= 1 << ( 5%32);
//     mask[57/32] |= 1 << (57%32);
//     STV_VerifyShared1(mask, true);
// note that these are incrementally verified, so calling this function twice on an SID won't reverify it


// == ERRORS ==
// all "int" functions above return 0 for success or a stack of negative-number errors on failure
// SysTitTag functions return SysTitTag.type ≥1 for a match or 0 for no match
// see internal.h for a list of errors and their sources


// == MANUALLY FREEING MEMORY ==
// this library stores, in memory for the duration of the program, the most recently loaded:
//   TMD, content file, shared content map
// TMDs are loaded by STV_LoadTitle, and content files are loaded just-in-time for an operation
// loading a new one frees the previous one, so users needn't worry about memory
// nonetheless, to free this memory completely, you can call these functions:
void STV_FreeTitle(void);       // frees TMD
void STV_FreeContent(void);     // frees content file and shared content map

#pragma GCC visibility pop
