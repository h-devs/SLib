/**
 * @file slib/storage/filesystemhost.h
 * FileSystem Host Layer Definition.
 *
 * @copyright 2020 Steve Han
 */

#pragma once

#include <slib/core.h>
#include "file_system.h"

namespace slib
{

	class FileSystemHost : public Object
	{
	public:
		FileSystemHost(Ref<FileSystemProvider> base) : m_base(base)
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
		virtual int fsRun() = 0;
		virtual int fsStop() = 0;
		virtual int isRunning() = 0;

	public:
		sl_size getOpenHandlesCount()
		{
			return m_base->getOpenHandlesCount();
		}

	protected:
		Ref<FileSystemProvider> m_base;
	};

}