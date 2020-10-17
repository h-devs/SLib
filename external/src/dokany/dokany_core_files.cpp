#include "dokany_core_files.h"

namespace dokany
{
	namespace files
	{

		static unsigned char dokan_arr_dll[] = {
#ifdef _WIN64
#include "dokan_x64_dll.txt"
#else
#include "dokan_x86_dll.txt"
#endif
		};
		unsigned char* dokan_dll_compressed_data = dokan_arr_dll;
		unsigned long dokan_dll_compressed_size = sizeof(dokan_arr_dll);

		static unsigned char dokan_arr_sys[] = {
#ifdef _WIN64
#include "dokan_x64_sys.txt"
#else
#include "dokan_x86_sys.txt"
#endif
		};
		unsigned char* dokan_sys_compressed_data = dokan_arr_sys;
		unsigned long dokan_sys_compressed_size = sizeof(dokan_arr_sys);

#ifndef _WIN64
		static unsigned char dokan_arr_sys64[] = {
#include "dokan_x64_sys.txt"
		};
		unsigned char* dokan_sys_compressed_data64 = dokan_arr_sys64;
		unsigned long dokan_sys_compressed_size64 = sizeof(dokan_arr_sys64);
#endif

		static unsigned char dokan1_arr_dll[] = {
#ifdef _WIN64
#include "dokan1_x64_dll.txt"
#else
#include "dokan1_x86_dll.txt"
#endif
		};
		unsigned char* dokan1_dll_compressed_data = dokan1_arr_dll;
		unsigned long dokan1_dll_compressed_size = sizeof(dokan1_arr_dll);

		static unsigned char dokan1_arr_sys[] = {
#ifdef _WIN64
#include "dokan1_x64_sys.txt"
#else
#include "dokan1_x86_sys.txt"
#endif
		};
		unsigned char* dokan1_sys_compressed_data = dokan1_arr_sys;
		unsigned long dokan1_sys_compressed_size = sizeof(dokan1_arr_sys);

#ifndef _WIN64
		static unsigned char dokan1_arr_sys64[] = {
#include "dokan1_x64_sys.txt"
		};
		unsigned char* dokan1_sys_compressed_data64 = dokan1_arr_sys64;
		unsigned long dokan1_sys_compressed_size64 = sizeof(dokan1_arr_sys64);
#endif

	}
}