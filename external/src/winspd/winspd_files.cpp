#include "winspd_files.h"

namespace winspd
{
	namespace files
	{

		static unsigned char _winspd_sys_x64[] = {
#include "x64/winspd_sys.txt"
		};
		unsigned char* winspd_sys_compressed_data64 = _winspd_sys_x64;
		unsigned long winspd_sys_compressed_size64 = sizeof(_winspd_sys_x64);

		static unsigned char _winspd_cat_x64[] = {
#include "x64/winspd_cat.txt"
		};
		unsigned char* winspd_cat_compressed_data64 = _winspd_cat_x64;
		unsigned long winspd_cat_compressed_size64 = sizeof(_winspd_cat_x64);

#ifndef _WIN64
		static unsigned char _winspd_sys_x86[] = {
#include "x86/winspd_sys.txt"
		};
		unsigned char* winspd_sys_compressed_data86 = _winspd_sys_x86;
		unsigned long winspd_sys_compressed_size86 = sizeof(_winspd_sys_x86);

		static unsigned char _winspd_cat_x86[] = {
#include "x86/winspd_cat.txt"
		};
		unsigned char* winspd_cat_compressed_data86 = _winspd_cat_x86;
		unsigned long winspd_cat_compressed_size86 = sizeof(_winspd_cat_x86);
#endif

	}
}