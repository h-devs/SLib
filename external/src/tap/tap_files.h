#ifndef CHECKHEADER_TAP_FILES
#define CHECKHEADER_TAP_FILES

namespace tap
{
	namespace files
	{
		
		extern unsigned char* tap_inf_compressed_data64;
		extern unsigned long tap_inf_compressed_size64;
		extern unsigned char* tap_sys_compressed_data64;
		extern unsigned long tap_sys_compressed_size64;
		extern unsigned char* tap_cat_compressed_data64;
		extern unsigned long tap_cat_compressed_size64;

#ifndef _WIN64
		extern unsigned char* tap_inf_compressed_data86;
		extern unsigned long tap_inf_compressed_size86;
		extern unsigned char* tap_sys_compressed_data86;
		extern unsigned long tap_sys_compressed_size86;
		extern unsigned char* tap_cat_compressed_data86;
		extern unsigned long tap_cat_compressed_size86;
		extern unsigned char* tapinstall_exe_compressed_data;
		extern unsigned long tapinstall_exe_compressed_size;
#endif

	}
}

#endif