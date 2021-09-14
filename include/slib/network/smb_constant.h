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

#ifndef CHECKHEADER_SLIB_NETWORK_SMB_CONSTANT
#define CHECKHEADER_SLIB_NETWORK_SMB_CONSTANT

#include "definition.h"

#include "../core/flags.h"

namespace slib
{

	enum class SmbCommand
	{
		Negotiate = 0x72
	};

	enum class SmbStatus : sl_uint32
	{
		Success = 0,
		Unsuccessful = 0xC0000001,
		MoreProcessingRequired = 0xc0000016,
		LoginFailure = 0xc000006d
	};

	enum class SmbDisposition
	{
		Supersede = 0, // File exists overwrite/supersede. File not exist create.
		Open = 1, // File exists open. File not exist fail.
		Create = 2, // File exists fail. File not exist create.
		OpenIf = 3, // File exists open. File not exist create.
		Overwrite = 4, // File exists overwrite. File not exist fail.
		OverwriteIf = 5 // File exists overwrite. File not exist create.
	};

	enum class SmbCreateAction
	{
		Existed = 1,
		Created = 2,
		Truncated = 3,
		Unknown = 5
	};

	SLIB_DEFINE_FLAGS(SmbAccessMask, {
		Read = 0x00000001,
		Write = 0x00000002,
		Append = 0x00000004,
		ReadExtendedAttributes = 0x00000008,
		WriteExtendedAttributes = 0x00000010,
		Execute = 0x00000020,
		DeleteChild = 0x00000040,
		ReadAttributes = 0x00000080,
		WriteAttributes = 0x00000100,
		Delete = 0x00010000,
		ReadControl = 0x00020000,
		ReadDAC = 0x00040000, // Discretionary access control
		WriteOwner = 0x00080000,
		Synchronize = 0x00100000,
		SystemSecurity = 0x01000000,
		MaximumAllowed = 0x02000000,
		GenericAll = 0x10000000
	})

	SLIB_DEFINE_FLAGS(SmbShareAccess, {
		None = 0,
		Read = 1,
		Write = 2,
		Delete = 4
	})

	SLIB_DEFINE_FLAGS(SmbCreateOptions, {
		Directory = 0x0001, // File being created/opened must be a directory
		WriteThrough = 0x0002, // Writes need flush buffered data before completing
		SequentialOnly = 0x0004, // The file might only be accessed sequentially
		NoIntermediateBuffering = 0x0008,
		SynchronousIoAlert = 0x0010, // may be ignored
		SynchronousIoNonAlert = 0x0020, // may be ignored
		NonDirectory = 0x0040, // File being created/opened must not be a directory
		CreateTreeConnection = 0x0080, // ignore, should be zero
		CompleteIfOplocked = 0x0100, // ignore, should be zero
		NoExtendedAttributesKnowledge = 0x0200, // The client don't understand Extended Attributes
		EightDotThreeOnly = 0x0400, // aka OPEN_FOR_RECOVERY: ignore, should be zero
		RandomAccess = 0x0800,
		DeleteOnClose = 0x1000, // The file should be deleted when it is closed
		OpenByFileId = 0x2000,
		OpenForBackupIntent = 0x4000,
		NoCompression = 0x8000,
		ReserverOpfilter = 0x00100000, // ignore, should be zero
		OpenReparsePoint = 0x00200000,
		OpenNoRecall = 0x00400000,
		OpenForFreeSpaceQuery = 0x00800000 // ignore should be zero
	})


	enum class Smb2Command
	{
		Negotiate = 0,
		SessionSetup = 1,
		LogOff = 2,
		TreeConnect = 3,
		TreeDisconnect = 4,
		Create = 5,
		Close = 6,
		Flush = 7,
		Read = 8,
		Write = 9,
		Lock = 10,
		Ioctl = 11,
		Cancel = 12,
		KeepAlive = 13,
		QueryDirectory = 14,
		Notify = 15,
		GetInfo = 16,
		SetInfo = 17,
		Break = 18
	};

	enum class Smb2ShareType
	{
		Disk = 1,
		NamedPipe = 2,
		Print = 3
	};

	enum class Smb2OplockLevel
	{
		None = 0,
		II = 1,
		Exclusive = 8,
		Batch = 9,
		Lease = 0xff
	};

	enum class Smb2ImpersonationLevel
	{
		Anonymouse = 0,
		Identification = 1,
		Impersonation = 2,
		Delegate = 3
	};

	enum class Smb2GetInfoClass
	{
		File = 1,
		FileSystem = 2,
		Security = 3,
		Quota = 4
	};

	enum class Smb2GetInfoLevel
	{
		FileStandardInfo = 5
	};

	SLIB_DEFINE_FLAGS(Smb2SessionFlags, {
		Guest = 0x0001,
		Null = 0x0002,
		Encrypt = 0x0004
	})

	SLIB_DEFINE_FLAGS(Smb2SecurityMode, {
		SigningEnabled = 1,
		SigningRequired = 2
	})

	SLIB_DEFINE_FLAGS(Smb2Capabilities, {
		DFS = 0x00000001,
		Leasing = 0x00000002,
		LargeMtu = 0x00000004,
		MultiChannel = 0x00000008,
		PersistentHandles = 0x00000010,
		DirectoryLeasing = 0x00000020,
		Encryption = 0x00000040
	})

	SLIB_DEFINE_FLAGS(Smb2ShareFlags, {
		DFS = 0x0001,
		DFSRoot = 0x0002,
		RestrictExclusiveOpens = 0x0100,
		ForceShareDelete = 0x0200,
		AllowNamespaceCaching = 0x0400,
		AccessBasedDirectoryEnum = 0x0800,
		ForceLevelIIOplock = 0x1000,
		EnableHashV1 = 0x2000,
		EnableHashV2 = 0x4000,
		EncryptData = 0x8000
	})

	SLIB_DEFINE_FLAGS(Smb2ShareCapabilities, {
		DFS = 0x08,
		ContinuousAvailability = 0x10,
		Scaleout = 0x20,
		Cluster = 0x40,
		Asymmetric = 0x80
	})

}

#endif
