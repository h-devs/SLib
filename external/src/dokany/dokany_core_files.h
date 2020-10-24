#ifndef CHECKHEADER_DOKANY_CORE_FILES
#define CHECKHEADER_DOKANY_CORE_FILES

namespace dokany
{
	namespace files
	{
		extern unsigned char* dokan_mounter_compressed_data;
		extern unsigned long dokan_mounter_compressed_size;

#ifndef _WIN64
		extern unsigned char* dokan_mounter_compressed_data64;
		extern unsigned long dokan_mounter_compressed_size64;
#endif

		extern unsigned char* dokan_dll_compressed_data;
		extern unsigned long dokan_dll_compressed_size;

		extern unsigned char* dokan_sys_compressed_data;
		extern unsigned long dokan_sys_compressed_size;

#ifndef _WIN64
		extern unsigned char* dokan_sys_compressed_data64;
		extern unsigned long dokan_sys_compressed_size64;
#endif

		extern unsigned char* dokan1_dll_compressed_data;
		extern unsigned long dokan1_dll_compressed_size;

		extern unsigned char* dokan1_sys_compressed_data;
		extern unsigned long dokan1_sys_compressed_size;

#ifndef _WIN64
		extern unsigned char* dokan1_sys_compressed_data64;
		extern unsigned long dokan1_sys_compressed_size64;
#endif

	}
}

#endif