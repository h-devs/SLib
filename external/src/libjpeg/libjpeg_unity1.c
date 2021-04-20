#include "jaricom.c"
#include "jcapimin.c"
#include "jcapistd.c"
#include "jcarith.c"
#include "jccoefct.c"
#include "jccolor.c"
#undef FIX
#include "jcdctmgr.c"
#define encode_mcu_DC_first jchuff_encode_mcu_DC_first
#define encode_mcu_AC_first jchuff_encode_mcu_AC_first
#define encode_mcu_DC_refine jchuff_encode_mcu_DC_refine
#define encode_mcu_AC_refine jchuff_encode_mcu_AC_refine
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

#include "jchuff.c"
#include "jcinit.c"
#include "jcmainct.c"
#define emit_byte jcmarker_emit_byte
#include "jcmarker.c"
#include "jcmaster.c"
#include "jcomapi.c"
#include "jcparam.c"
#include "jcprepct.c"
#include "jcsample.c"
#define my_coef_controller jctrans_my_coef_controller
#define my_coef_ptr jctrans_my_coef_ptr
#define start_iMCU_row jctrans_start_iMCU_row
#define start_pass_coef jctrans_start_pass_coef
#define compress_output jctrans_compress_output
#include "jctrans.c"