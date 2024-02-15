#include "winhook.h"

#include "input_dll_file.h"

#include <slib/core/string.h>
#include <slib/core/memory.h>
#include <slib/io/file.h>
#include <slib/data/zstd.h>

namespace slib
{

	sl_bool HookInput::install()
	{
		Memory data = Zstd::decompress(winhook::files::input_dll_data, winhook::files::input_dll_size);
		String path = getDllPath();
		if (File::readAllBytes(path) == data) {
			return sl_true;
		}
		File::createDirectories(File::getParentDirectoryPath(path));
		return File::writeAllBytes(path, data) == data.getSize();
	}

}