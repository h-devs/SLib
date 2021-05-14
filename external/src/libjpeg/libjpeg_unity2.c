#if defined(_MSC_VER)
#pragma optimize("t", on)
#pragma warning(disable: 4996)
#pragma warning(disable: 4267)
#elif defined(__GNUC__)
#pragma GCC optimize ("O3")
#ifdef __clang__
#pragma GCC diagnostic ignored "-Wcomma"
#endif
#endif

#include "jdapimin.c"
#include "jdapistd.c"
#include "jdarith.c"
#include "jdatadst.c"
#include "jdatasrc.c"
#include "jdcoefct.c"
#include "jdcolor.c"
#undef FIX
#include "jddctmgr.c"
#define process_restart jdhuff_process_restart
#define decode_mcu_DC_first jdhuff_decode_mcu_DC_first
#define decode_mcu_AC_first jdhuff_decode_mcu_AC_first
#define decode_mcu_DC_refine jdhuff_decode_mcu_DC_refine
#define decode_mcu_AC_refine jdhuff_decode_mcu_AC_refine
#include "jdhuff.c"
#include "jdinput.c"
#include "jdmainct.c"
#include "jdmarker.c"
#include "jdmaster.c"
#undef FIX
#define build_ycc_rgb_table jdmerge_build_ycc_rgb_table
#define build_bg_ycc_rgb_table jdmerge_build_bg_ycc_rgb_table
#include "jdmerge.c"
#include "jdpostct.c"
#define my_upsampler jdsample_my_upsampler
#define my_upsample_ptr jdsample_my_upsample_ptr
#undef FIX
#include "jdsample.c"
#include "jdtrans.c"
#include "jerror.c"
#include "jfdctflt.c"
#undef CONST_BITS
#include "jfdctfst.c"
#undef FIX_0_541196100
#undef FIX_1_847759065
#undef MULTIPLY
#undef CONST_BITS
#include "jfdctint.c"
#include "jidctflt.c"
#undef CONST_BITS
#undef FIX_1_847759065
#undef MULTIPLY
#undef DEQUANTIZE
#include "jidctfst.c"
#undef CONST_BITS
#undef FIX_1_847759065
#undef MULTIPLY
#undef DEQUANTIZE
#include "jidctint.c"
#include "jmemmgr.c"
#include "jmemnobs.c"
#include "jquant1.c"
#define my_cquantizer jquant2_my_cquantizer
#define my_cquantize_ptr jdquant2_my_cquantize_ptr
#include "jquant2.c"
#include "jutils.c"