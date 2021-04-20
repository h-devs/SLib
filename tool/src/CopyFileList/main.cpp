#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	if (argc != 4) {
		Println("Usage: CopyFileList [Source Directory] [Target Directory] [List File]");
		return -1;
	}

	String pathSrc = argv[1];
	String pathDst = argv[2];
	String pathList = argv[3];

	if (!(File::isDirectory(pathSrc))) {
		Println("Source Directory is invalid: %s", pathSrc);
		return -1;
	}
	if (!(File::isDirectory(pathDst))) {
		Println("Target Directory is invalid: %s", pathDst);
		return -1;
	}
	if (!(File::isFile(pathList))) {
		Println("List File is invalid: %s", pathList);
		return -1;
	}

	Println("Source Directory: %s", pathSrc);
	Println("Target Directory: %s", pathDst);
	Println("List File: %s", pathList);

	auto fileList = File::openForRead(pathList);
	if (fileList.isNull()) {
		Println("Failed to open list file!");
		return -1;
	}

	while (!(fileList->isEnd())) {
		String item = fileList->readLine().trim();
		if (item.isNotEmpty()) {
			String pathSrcFile = File::joinPath(pathSrc, item);
			if (File::exists(pathSrcFile)) {
				String pathDstFile = File::joinPath(pathDst, item);
				File::createDirectories(File::getParentDirectoryPath(pathDstFile));
				if (File::copyFile(pathSrcFile, pathDstFile)) {
					Println("Copied: %s", item);
				} else {
					Println("Failed to copy: %s", item);
				}
			} else {
				Println("File not found: %s", item);
			}
		}
	}
	return 0;
}
