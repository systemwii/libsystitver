// test script for this library
#include <stdio.h>
#include <unistd.h>
#include <ogc/lwp_watchdog.h>
#include "console.h"	// from monke
#include "systitver.h"

char* errStr = "";
int ret = 0;

int timer() {	// these timer numbers are all wong :c
	volatile static int t1, t2 = 0;
	t1 = t2;
	t2 = gettime();
	return diff_msec(t1, t2);
}

int fail() {
	printf(CON_RED("ERROR | %s [%d]\n"), errStr, ret);
	sleep(5); return 0;
}

// test for caching shared1 verification results 
void testCaching(bool log) {
	u32 maskA[4] = {};
	maskA[23/32] |= 1 << (23%32);
	maskA[26/32] |= 1 << (26%32);
	ret = STV_VerifyShared1(maskA, true);
	if (ret != 0) { errStr = "(STV_Shared1)"; fail(); return; }

	u32 maskB[4] = {};
	maskB[23/32] |= 1 << (23%32);
	maskB[40/32] |= 1 << (40%32);
	ret = STV_VerifyShared1(maskB, true);
	if (ret != 0) { errStr = "(STV_Shared1)"; fail(); return; }
	return;
}

void testTitle(int id, bool log) {
	ret = STV_LoadTitle(id, log);
	if (ret != 0) {errStr = "(STV_LoadTitle)"; fail(); return;}

	ret = STV_VerifyCurrentTitle(log);
	if (ret != 0) {errStr = "(STV_VerifyCurrentTitle)"; fail(); return;}

	SysTitTag stt = STV_IdentifyCurrentTitleByHash(log);
	if (log && stt.titleID < 0) {printf("> not tmd matched (%d)\n", stt.titleID);}

	if (id == 257) {
		stt = STV_IdentifyMIOSBySize(log);
		if (log && stt.titleID < 0) {printf("> not size matched (%d)\n", stt.titleID);}
	}
	if (log) {sleep(1);}
}

// system menu is much slower than all others
void testAllExceptSystemMenu(bool log) {
	static int n[51] = {
		 3, 4, 9,10,11,    12,13,14,15,16,		17,20,21,22,28,	   30,31,33,34,35,
		36,37,38,40,41,	   43,45,46,48,50,		51,52,53,55,56,	   57,58,60,61,62,
		70,80,222,223,236, 249,250,251,254,256,	257
	};

	for (int k=0; k<51; k++) { testTitle(n[k], log); }
}


int main(int argc, char* argv[]) {
	consoleInit();

	printf("henlo\n\n");
	bool log = true;
	timer();
	// testCaching(log);			printf("cache test\n"); sleep(2);
	testAllExceptSystemMenu(log);	printf("ioses done in %d\n", timer());
	testTitle(2, log);				printf("system menu done in %d\n", timer());
	
	sleep(15); return 0;
}


// prototype code

/* // enumerates all system titles, could be used to verify all system titles
int test1() {
	u32 titleCount;
	ret = ES_GetNumTitles(&titleCount);
	if (ret != 0) { return ret; }
	printf("%d titles\n", titleCount);

	u64* titles = memalign(32, (sizeof(u64) * (s32)titleCount));
	if (titles == NULL) { errStr = "memalign failed"; return -32; }

	ret = ES_GetTitles(titles, (s32)titleCount);
	if (ret != 0) { errStr = "(ES_GetTitles)"; return ret; }

	for (s32 i = 0; i < titleCount; i++) {
		if (titles[i] >> 32 != 1) { continue; }
		printf("%08llx %03u\n", titles[i] >> 32 , (u32)(titles[i] & 0xFFFFFFFF));
		usleep(500000);
	}

	free(titles);
    return 0;
}
*/
