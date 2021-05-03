#include "tap_files.h"

namespace tap
{
	namespace files
	{

		static unsigned char _tap_inf_x64[] = {
#include "x64/tap0901_inf.txt"
		};
		unsigned char* tap_inf_compressed_data64 = _tap_inf_x64;
		unsigned long tap_inf_compressed_size64 = sizeof(_tap_inf_x64);

		static unsigned char _tap_sys_x64[] = {
#include "x64/tap0901_sys.txt"
		};
		unsigned char* tap_sys_compressed_data64 = _tap_sys_x64;
		unsigned long tap_sys_compressed_size64 = sizeof(_tap_sys_x64);

		static unsigned char _tap_cat_x64[] = {
#include "x64/tap0901_cat.txt"
		};
		unsigned char* tap_cat_compressed_data64 = _tap_cat_x64;
		unsigned long tap_cat_compressed_size64 = sizeof(_tap_cat_x64);


#ifndef _WIN64
		static unsigned char _tap_inf_x86[] = {
#include "x86/tap0901_inf.txt"
		};
		unsigned char* tap_inf_compressed_data86 = _tap_inf_x86;
		unsigned long tap_inf_compressed_size86 = sizeof(_tap_inf_x86);

		static unsigned char _tap_sys_x86[] = {
#include "x86/tap0901_sys.txt"
		};
		unsigned char* tap_sys_compressed_data86 = _tap_sys_x86;
		unsigned long tap_sys_compressed_size86 = sizeof(_tap_sys_x86);

		static unsigned char _tap_cat_x86[] = {
#include "x86/tap0901_cat.txt"
		};
		unsigned char* tap_cat_compressed_data86 = _tap_cat_x86;
		unsigned long tap_cat_compressed_size86 = sizeof(_tap_cat_x86);
		
		static unsigned char _tapinstall_exe[] = {
#include "x64/tapinstall_exe.txt"
		};
		unsigned char* tapinstall_exe_compressed_data = _tapinstall_exe;
		unsigned long tapinstall_exe_compressed_size = sizeof(_tapinstall_exe);
#endif

	}
}