#include <slib.h>

using namespace slib;

void PrintUsage()
{
	Println("Usage: chacha {d|e} [key] [source path] [destination path]");;
}

sl_bool DoFileOperation(sl_bool flagEncrypt, const String& key, const String& pathSrc, const String& pathDst)
{
	Println("%s -> %s", pathSrc, pathDst);
	auto fileSrc = File::openForRead(pathSrc);
	if (fileSrc.isNull()) {
		Println("Failed to open for read: %s", pathSrc);
		return sl_false;
	}
	char header[ChaCha20FileEncryptor::HeaderSize];
	ChaCha20FileEncryptor enc;
	if (flagEncrypt) {
		enc.create(header, key.getData(), (sl_uint32)(key.getLength()));
	} else {
		if (fileSrc->readFully(header, sizeof(header)) != sizeof(header)) {
			Println("Invalid header size: %s", pathSrc);
			return sl_false;
		}
		if (!(enc.open(header, key.getData(), (sl_uint32)(key.getLength())))) {
			Println("Invalid key on file: %s", pathSrc);
			return sl_false;
		}
	}
	auto fileDst = File::openForWrite(pathDst);
	if (fileDst.isNull()) {
		Println("Failed to open for write: %s", pathDst);
		return sl_false;
	}
	if (flagEncrypt) {
		if (fileDst->writeFully(header, sizeof(header)) != sizeof(header)) {
			Println("Failed to write header: %s", pathDst);
			return sl_false;
		}
	}
	static char buf[1024 * 1024];
	sl_size offset = 0;
	for (;;) {
		if (fileSrc->isEnd()) {
			return sl_true;
		}
		sl_reg n = fileSrc->read(buf, sizeof(buf));
		if (n < 0) {
			Println("Failed to read data: %s", pathSrc);
			break;
		}
		if (n) {
			enc.encrypt(offset, buf, buf, n);
			if (fileDst->writeFully(buf, n) != n) {
				Println("Failed to write data: %s", pathDst);
				break;
			}
			offset += n;
		}
	}
	fileDst->close();
	File::deleteFile(pathDst);
	return sl_false;
}

sl_bool DoDirOperation(sl_bool flagEncrypt, const String& key, const String& pathSrc, const String& pathDst)
{
	for (auto& fileName : File::getFiles(pathSrc)) {
		String pathSrcFile = pathSrc + "/" + fileName;
		String pathDstFile = pathDst + "/" + fileName;
		FileAttributes attrs = File::getAttributes(pathSrcFile);
		if (!(attrs & FileAttributes::NotExist)) {
			if (attrs & FileAttributes::Directory) {
				if (File::createDirectory(pathDstFile)) {
					DoDirOperation(flagEncrypt, key, pathSrcFile, pathDstFile);
				} else {
					Println("Failed to create directory: %s", pathDstFile);
				}
			} else {
				DoFileOperation(flagEncrypt, key, pathSrcFile, pathDstFile);
			}
		}
	}
	return sl_true;
}

int main(int argc, const char * argv[])
{
	if (argc != 5) {
		PrintUsage();
		return -1;
	}
	String method = argv[1];
	String key = argv[2];
	String pathSrc = argv[3];
	String pathDst = argv[4];
	sl_bool flagEncrypt = sl_false;
	if (method == "e") {
		flagEncrypt = sl_true;
	} else if (method == "d") {
		flagEncrypt = sl_false;
	} else {
		PrintUsage();
		return -1;
	}
	sl_bool flagDir = sl_false;
	if (File::isDirectory(pathSrc)) {
		if (File::isDirectory(pathDst)) {
			flagDir = sl_true;
		} else {
			Println("Destination is not directory!");
			return -1;
		}
	} else {
		if (File::isDirectory(pathDst)) {
			pathDst += "/" + File::getFileName(pathSrc);
		}
	}
	if (flagDir) {
		if (!(DoDirOperation(flagEncrypt, key, pathSrc, pathDst))) {
			return -1;
		}
	} else {
		if (!(DoFileOperation(flagEncrypt, key, pathSrc, pathDst))) {
			return -1;
		}
	}
	return 0;
}
