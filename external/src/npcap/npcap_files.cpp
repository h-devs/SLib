#include "npcap_files.h"

namespace npcap
{
	namespace files
	{

		static unsigned char _npcap_inf_x64[] = {
#include "x64/npcap_inf.txt"
		};
		unsigned char* npcap_inf_compressed_data64 = _npcap_inf_x64;
		unsigned long npcap_inf_compressed_size64 = sizeof(_npcap_inf_x64);

		static unsigned char _npcap_sys_x64[] = {
#include "x64/npcap_sys.txt"
		};
		unsigned char* npcap_sys_compressed_data64 = _npcap_sys_x64;
		unsigned long npcap_sys_compressed_size64 = sizeof(_npcap_sys_x64);

		static unsigned char _npcap_cat_x64[] = {
#include "x64/npcap_cat.txt"
		};
		unsigned char* npcap_cat_compressed_data64 = _npcap_cat_x64;
		unsigned long npcap_cat_compressed_size64 = sizeof(_npcap_cat_x64);

		static unsigned char _npfinstall_exe_x64[] = {
#include "x64/npfinstall_exe.txt"
		};
		unsigned char* npfinstall_exe_compressed_data64 = _npfinstall_exe_x64;
		unsigned long npfinstall_exe_compressed_size64 = sizeof(_npfinstall_exe_x64);
		
#ifndef _WIN64
		static unsigned char _npcap_inf_x86[] = {
#include "x86/npcap_inf.txt"
		};
		unsigned char* npcap_inf_compressed_data86 = _npcap_inf_x86;
		unsigned long npcap_inf_compressed_size86 = sizeof(_npcap_inf_x86);

		static unsigned char _npcap_sys_x86[] = {
#include "x86/npcap_sys.txt"
		};
		unsigned char* npcap_sys_compressed_data86 = _npcap_sys_x86;
		unsigned long npcap_sys_compressed_size86 = sizeof(_npcap_sys_x86);

		static unsigned char _npcap_cat_x86[] = {
#include "x86/npcap_cat.txt"
		};
		unsigned char* npcap_cat_compressed_data86 = _npcap_cat_x86;
		unsigned long npcap_cat_compressed_size86 = sizeof(_npcap_cat_x86);

		static unsigned char _npfinstall_exe_x86[] = {
#include "x86/npfinstall_exe.txt"
		};
		unsigned char* npfinstall_exe_compressed_data86 = _npfinstall_exe_x86;
		unsigned long npfinstall_exe_compressed_size86 = sizeof(_npfinstall_exe_x86);
#endif

	}
}