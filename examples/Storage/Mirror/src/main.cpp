#include <slib.h>
#include <slib/platform.h>
#include <slib/storage/dokany.h>
#include <slib/storage/file_system_mirror.h>
#include <slib/storage/file_system_logger.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	if (argc < 3) {
		Println("Usage: %s rootdir mountpoint [logflags] [logfilter]", File::getFileName(argv[0]));
		return 0;
	}

	Dokany::install();

	String rootPath = StringCstr(argv[1]);
	String mountPoint = StringCstr(argv[2]);

	Ref<FileSystemProvider> rootFs = MirrorFileSystem::create(rootPath);

	// name wrapper
	rootFs = new FileSystemWrapper(rootFs, "MirrorFS", "");

	// logger
	sl_uint32 logFlags = 0;
	String logFilter = ".*";

	if (argc > 3 && String(argv[3]).getLength() == 8) {
		String::parseUint32(16, &logFlags, argv[3]);
		if (argc > 4) logFilter = argv[4];
	}
	rootFs = new FileSystemLogger(rootFs, logFlags, logFilter);

	Ref<FileSystemHost> host = FileSystem::createHost();
	FileSystemHostParam hostParam;
	hostParam.provider = rootFs;
	hostParam.mountPoint = mountPoint;

	if (!host->run(hostParam)) {
		Println("Error: %s", host->getErrorMessage());
		return 1;
	}
	return 0;
}
