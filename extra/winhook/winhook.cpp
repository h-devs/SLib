#include "winhook.h"

#include <slib/core/string.h>
#include <slib/core/safe_static.h>
#include <slib/system/system.h>
#include <slib/system/dynamic_library.h>
#include <slib/io/file.h>

#include "input_dll/api.h"

namespace slib
{

	namespace
	{
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicString, g_dllDir)
		SLIB_GLOBAL_ZERO_INITIALIZED(AtomicString, g_dllFileName)
		SLIB_GLOBAL_ZERO_INITIALIZED(DynamicLibrary, g_lib)
		SLIB_STATIC_SPINLOCK(g_lockLib)
	}

	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(HookInputParam)

	HookInputParam::HookInputParam(): flagBlockKeyboard(sl_false)
	{
	}

	String HookInput::getDllPath()
	{
		String dir = g_dllDir;
		if (dir.isEmpty()) {
			dir = System::getLocalAppDataDirectory();
		}
		String name = g_dllFileName;
		if (name.isEmpty()) {
			SLIB_STATIC_STRING(defaultName, "hook_input")
			name = defaultName;
		}
#ifdef SLIB_ARCH_IS_X64
		name += "64.dll";
#else
		name += "86.dll";
#endif
		return File::concatPath(dir, name);
	}

	void HookInput::setDllPath(const String& dir, const String& fileName)
	{
		g_dllDir = dir;
		g_dllFileName = fileName;
	}

	void HookInput::setDllDirectory(const String& dir)
	{
		g_dllDir = dir;
	}

	void HookInput::setDllName(const String& fileName)
	{
		g_dllFileName = fileName;
	}

	sl_bool HookInput::start(const HookInputParam& param)
	{
		SpinLocker lock(&g_lockLib);
		if (!(g_lib.isLoaded())) {
			if (!(g_lib.load(getDllPath()))) {
				return sl_false;
			}
		}
		SLIB_STATIC_STRING16(funcName, "StartHook")
		TYPE_StartHook func = (TYPE_StartHook)(g_lib.getFunctionAddress(funcName));
		if (func) {
			return func(param);
		} else {
			return sl_false;
		}
	}

	sl_bool HookInput::start(const HookInputCallback& callback)
	{
		HookInputParam param;
		param.onInput = callback;
		return start(param);
	}

	void HookInput::stop()
	{
		SpinLocker lock(&g_lockLib);
		if (!(g_lib.isLoaded())) {
			return;
		}
		SLIB_STATIC_STRING16(funcName, "StopHook")
		TYPE_StopHook func = (TYPE_StopHook)(g_lib.getFunctionAddress(funcName));
		if (func) {
			func();
		} else {
			return;
		}
		g_lib.free();
	}

}