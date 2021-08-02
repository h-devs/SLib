#ifndef CHECKHEADER_WINSPD_FILES
#define CHECKHEADER_WINSPD_FILES

namespace winspd
{
	namespace files
	{

		extern unsigned char* winspd_sys_compressed_data64;
		extern unsigned long winspd_sys_compressed_size64;
		extern unsigned char* winspd_cat_compressed_data64;
		extern unsigned long winspd_cat_compressed_size64;

#ifndef _WIN64
		extern unsigned char* winspd_sys_compressed_data86;
		extern unsigned long winspd_sys_compressed_size86;
		extern unsigned char* winspd_cat_compressed_data86;
		extern unsigned long winspd_cat_compressed_size86;
#endif

	}
}

#endif