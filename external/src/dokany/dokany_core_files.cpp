#include "dokany_core_files.h"

namespace dokany
{
	namespace files
	{

		static unsigned char arr_dll[] = {
#ifdef _WIN64
#include "dokan1_x64_dll.txt"
#else
#include "dokan1_x86_dll.txt"
#endif
		};
		unsigned char* dll_compressed_data = arr_dll;
		unsigned long dll_compressed_size = sizeof(arr_dll);

		static unsigned char arr_sys[] = {
#ifdef _WIN64
#include "dokan1_x64_sys.txt"
#else
#include "dokan1_x86_sys.txt"
#endif
		};
		unsigned char* sys_compressed_data = arr_sys;
		unsigned long sys_compressed_size = sizeof(arr_sys);

#ifndef _WIN64
		static unsigned char arr_sys64[] = {
#include "dokan1_x64_sys.txt"
		};
		unsigned char* sys_compressed_data64 = arr_sys64;
		unsigned long sys_compressed_size64 = sizeof(arr_sys64);
#endif

	}
}