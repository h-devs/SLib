#ifndef CHECKHEADER_DOKANY_CORE_FILES
#define CHECKHEADER_DOKANY_CORE_FILES

namespace dokany
{
	namespace files
	{

		extern unsigned char* dll_compressed_data;
		extern unsigned long dll_compressed_size;

		extern unsigned char* sys_compressed_data;
		extern unsigned long sys_compressed_size;

#ifndef _WIN64
		extern unsigned char* sys_compressed_data64;
		extern unsigned long sys_compressed_size64;
#endif

	}
}

#endif