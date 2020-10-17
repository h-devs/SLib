/**
 * @file slib/filesystem/filesystemhost.h
 * FileSystem Host Layer Definition.
 *
 * @copyright 2020 Steve Han
 */

#pragma once

#include <slib/core.h>
#include "filesystembase.h"

namespace slib
{

	class FileSystemHost : public Object
	{
	public:
		FileSystemHost(Ref<FileSystemBase> base) : _BaseFs(base)
		{
			if (base.isNull()) {
				Logger::logGlobalError("FileSystemHost", "BaseFs cannot be empty.");
				throw FileSystemError::InitFailure;
			}
		}
			
		virtual ~FileSystemHost()
		{
		}

	public:
		virtual int Run() = 0;
		virtual int Stop() = 0;
		virtual int IsRunning() = 0;

	public:
		sl_size getOpenHandlesCount()
		{
			return _BaseFs->getOpenHandlesCount();
		}

	protected:
		Ref<FileSystemBase> _BaseFs;
	};

}