#ifndef CHECKHEADER_DOKANY_FILES
#define CHECKHEADER_DOKANY_FILES

namespace dokany
{
	namespace files
	{

		extern unsigned char* dokan1_sys_compressed_data64;
		extern unsigned long dokan1_sys_compressed_size64;
		extern unsigned char* dokan1_cat_compressed_data64;
		extern unsigned long dokan1_cat_compressed_size64;

#ifndef _WIN64
		extern unsigned char* dokan1_sys_compressed_data86;
		extern unsigned long dokan1_sys_compressed_size86;
		extern unsigned char* dokan1_cat_compressed_data86;
		extern unsigned long dokan1_cat_compressed_size86;
#endif

	}
}

#endif