#ifndef CHECKHEADER_NPCAP_FILES
#define CHECKHEADER_NPCAP_FILES

namespace npcap
{
	namespace files
	{
		
		extern unsigned char* npcap_inf_compressed_data64;
		extern unsigned long npcap_inf_compressed_size64;
		extern unsigned char* npcap_sys_compressed_data64;
		extern unsigned long npcap_sys_compressed_size64;
		extern unsigned char* npcap_cat_compressed_data64;
		extern unsigned long npcap_cat_compressed_size64;
		extern unsigned char* npfinstall_exe_compressed_data64;
		extern unsigned long npfinstall_exe_compressed_size64;

#ifndef _WIN64
		extern unsigned char* npcap_inf_compressed_data86;
		extern unsigned long npcap_inf_compressed_size86;
		extern unsigned char* npcap_sys_compressed_data86;
		extern unsigned long npcap_sys_compressed_size86;
		extern unsigned char* npcap_cat_compressed_data86;
		extern unsigned long npcap_cat_compressed_size86;
		extern unsigned char* npfinstall_exe_compressed_data86;
		extern unsigned long npfinstall_exe_compressed_size86;
#endif

	}
}

#endif