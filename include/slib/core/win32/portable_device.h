/*
 *   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#ifndef CHECKHEADER_SLIB_CORE_WIN32_PORTABLE_DEVICE
#define CHECKHEADER_SLIB_CORE_WIN32_PORTABLE_DEVICE

#include "../definition.h"

#ifdef SLIB_PLATFORM_IS_WIN32

#include "com.h"

#include <PortableDeviceApi.h>

/*
	Note: Don't forget to call `CoInitialize()` before using `PortableDevice`
*/

namespace slib
{

	namespace win32
	{

		enum class PortableDeviceType
		{
			Unknown = 0,
			Generic = 1,
			Camera = 2,
			MediaPlayer = 3,
			Phone = 4,
			Video = 5,
			PersonalInformationManager = 6,
			AudioRecorder = 7
		};

		struct SLIB_EXPORT PortableDeviceInfo
		{
			String id;
			String name;
			String description;
			String manufacturer;
		};

		struct SLIB_EXPORT PortableDeviceObjectInfo
		{
			String id;
			String name;
		};

		class SLIB_EXPORT PortableDeviceManager
		{
			SLIB_DECLARE_WIN32_COM_CONTAINER_MEMBERS(PortableDeviceManager, IPortableDeviceManager, m_object)

		public:
			static PortableDeviceManager create();

		public:
			List<String> getDeviceIdentifiers();

			List<PortableDeviceInfo> getDeviceInfos();

		};

		class SLIB_EXPORT PortableDeviceProperties
		{
			SLIB_DECLARE_WIN32_COM_CONTAINER_MEMBERS(PortableDeviceProperties, IPortableDeviceProperties, m_object)

		public:
			String getObjectName(const StringParam& id);

		};

		class SLIB_EXPORT PortableDeviceContent
		{
			SLIB_DECLARE_WIN32_COM_CONTAINER_MEMBERS(PortableDeviceContent, IPortableDeviceContent, m_object)

		public:
			List<String> getObjectIdentifiers(const StringParam& parentId);

			List<String> getObjectIdentifiers();

			PortableDeviceProperties getProperties();

			List<PortableDeviceObjectInfo> getObjectInfos(const StringParam& parentId);

			List<PortableDeviceObjectInfo> getObjectInfos();

		};

		class SLIB_EXPORT PortableDevice
		{
			SLIB_DECLARE_WIN32_COM_CONTAINER_MEMBERS(PortableDevice, IPortableDevice, m_object)

		public:
			static List<String> getDeviceIdentifiers();

			static List<PortableDeviceInfo> getDeviceInfos();

			static PortableDevice open(const StringParam& id);

		public:
			PortableDeviceContent getContent();

			List<PortableDeviceObjectInfo> getObjectInfos(const StringParam& parentId);

			List<PortableDeviceObjectInfo> getObjectInfos();

			PortableDeviceType getType();

			String getProtocol();

		};

	}

}

#endif

#endif