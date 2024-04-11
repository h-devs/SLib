#include "input_dll_file.h"

namespace winhook
{
	namespace files
	{
		static unsigned char _input_dll[] = {
#ifdef _WIN64
#include "input_dll_x64.txt"
#else
#include "input_dll_x86.txt"
#endif
		};
		unsigned char* input_dll_data = _input_dll;
		unsigned long input_dll_size = sizeof(_input_dll);
	}
}