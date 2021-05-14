#include "png.c"
#include "pngerror.c"
#include "pngget.c"
#include "pngmem.c"
#include "pngpread.c"
#include "pngread.c"
#include "pngrio.c"
#include "pngrtran.c"
#include "pngrutil.c"
#include "pngset.c"
#include "pngtrans.c"
#include "pngwio.c"
#include "pngwrite.c"
#include "pngwtran.c"
#include "pngwutil.c"

#if defined(__amd64__) || defined(__x86_64__) || defined(_M_X64) || defined(_M_AMD64) || defined(i386) || defined(__i386) || defined(__i386__) || defined(_X86_) || defined(_M_IX86) || defined(EMSCRIPTEN)
#include "intel/intel_init.c"
#include "intel/filter_sse2_intrinsics.c"
#endif

#if defined(__arm__) || defined(__arm) || defined(ARM) || defined(_ARM_) || defined(__ARM__) || defined(_M_ARM)
#include "arm/arm_init.c"
#include "arm/filter_neon_intrinsics.c"
#endif