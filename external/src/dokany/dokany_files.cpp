#include "dokany_files.h"

namespace dokany
{
	namespace files
	{

		static unsigned char _dokan_sys_x64[] = {
#include "x64/dokan1_sys.txt"
		};
		unsigned char* dokan1_sys_compressed_data64 = _dokan_sys_x64;
		unsigned long dokan1_sys_compressed_size64 = sizeof(_dokan_sys_x64);

		static unsigned char _dokan_cat_x64[] = {
#include "x64/dokan1_cat.txt"
		};
		unsigned char* dokan1_cat_compressed_data64 = _dokan_cat_x64;
		unsigned long dokan1_cat_compressed_size64 = sizeof(_dokan_cat_x64);

#ifndef _WIN64
		static unsigned char _dokan_sys_x86[] = {
#include "x86/dokan1_sys.txt"
		};
		unsigned char* dokan1_sys_compressed_data86 = _dokan_sys_x86;
		unsigned long dokan1_sys_compressed_size86 = sizeof(_dokan_sys_x86);

		static unsigned char _dokan_cat_x86[] = {
#include "x86/dokan1_cat.txt"
		};
		unsigned char* dokan1_cat_compressed_data86 = _dokan_cat_x86;
		unsigned long dokan1_cat_compressed_size86 = sizeof(_dokan_cat_x86);
#endif

	}
}