# libsystitver : System Title Integrity Checking

A library for Wii homebrew to check the integrity of system titles (all IOSes, BC+MIOS and the System Menu).

### Wii System Title Verification In Brief

The available titles to verify are the ones located at `nand:/title/00000001/`. Their folders are their IDs (= titleIdLower) encoded as 8-digit hexadecimal numbers (e.g. IOS 36 (`titleIdLower=36`) is at `nand:/title/00000001/00000024`). These are every IOS, as well as BC, MIOS and the System Menu.

Each has a TMD (title metadata file), which lists its content files. The content files are either unique to the title (in its folder), or shared (in `nand:/shared1`). The TMD specifies each content file by its ID and SHA1 hash:
- unique ones are looked up by ID,
- shared ones are looked up by hash in the shared content map (`nand:/shared1/content.map`).

So to verify the integrity of a title, we resolve each shared content file using this map, resolve unique content files by their ID, hash them all and compare to the TMD hashes, then finally hash the TMD and identify it (see [identify.h](src/identify.h) for the reference table). Now, you know enuff to understand what this library does 🤝.

Looking up hashes of TMDs is one of several possible heuristics for identifying titles, which can be implemented as new `STV_Identify` functions here. Another that is implemented here is hashing the content metadata block of the TMD only (from offset 0x1e4 to end), which can be useful for identifying custom IOSes/MIOSes whose content is the same but tend to have cosmetic differences in the rest of the TMD (such as revision number). This doesn't identify ones that have the same content files at different IDs or in different orders, tho; that could be added later.

### Usage

See src/systitver.h.

### Setup

This library is designed to be included in a Git repository and compiled alongside the rest of the program. It works out-of-the-box with the [systemwii Make setup](https://github.com/systemwii/make) but can also be adapted to other build setups.

1. Add it as a submodule:
```bash
git submodule add https://github.com/systemwii/libsystitver.git lib/systitver
```
2. Set the version you want:
```bash
cd lib/systitver && git checkout <branch/tag/commit> && cd ../..
```
3. Ensure your Makefile can find it with:
```makefile
    LIBDIRSBNDLE =	$(wildcard lib/*/_)
```
4. Use it in relevant source files with:
```c
#include "systitver.h"
```
5. Build it or clear built files with these commands, in either its own folder or those of its containing projects:
```bash
make
make clean
```

Update your copy of this repo the usual way:
```bash
cd lib/systitver && git pull && cd ../..
```

### Building

This library is included by source and compiled alongside applications. To clone it independently, use the command:
```bash
git clone --recurse-submodules https://github.com/systemwii/libsystitver.git libsystitver
```
Update any of its submodules, e.g. "make" here, with:
```bash
cd lib/make && git pull && cd ../..
```
Or update them all with:
```bash
git submodule update --remote
```
