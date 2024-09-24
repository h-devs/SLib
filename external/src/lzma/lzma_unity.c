static void *MyAlloc(size_t size);
static void MyFree(void *address);

#include "Alloc.c"
#include "LzFind.c"
#include "LzHash.h"
#include "LzmaDec.c"
#undef kNumFullDistances
#define kLiteralNextStates enc_kLiteralNextStates
#include "LzmaEnc.c"
#include "LzmaLib.c"
