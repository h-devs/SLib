#include <slib.h>

using namespace slib;

void PrintUsage()
{
	Println("Usage: chacha {d|e} [key] [source path] [destination path]");
	Println("       chacha c [key] [path]   ; Check Password");
	Println("       chacha u [original key] [new key] [path]   ; Update Password");
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

sl_bool CheckPassword(const String& key, const String& path)
{
	auto file = File::openForRead(path);
	if (file.isNull()) {
		Println("Failed to open for read: %s", path);
		return sl_false;
	}
	char header[ChaCha20FileEncryptor::HeaderSize];
	if (file->readFully(header, sizeof(header)) != sizeof(header)) {
		Println("Invalid header size: %s", path);
		return sl_false;
	}
	if (ChaCha20FileEncryptor::checkPassword(header, key.getData(), (sl_uint32)(key.getLength()))) {
		Println("OK!");
		return sl_true;
	} else {
		Println("Invalid Key!");
		return sl_false;
	}
}

sl_bool UpdateFilePassword(const String& oldKey, const String& newKey, const String& path)
{
	Println("Processing: %s", path);
	auto file = File::open(path, FileMode::ReadWrite | FileMode::NotCreate | FileMode::NotTruncate);
	if (file.isNull()) {
		Println("Failed to open for read: %s", path);
		return sl_false;
	}
	char header[ChaCha20FileEncryptor::HeaderSize];
	if (file->readFully(header, sizeof(header)) != sizeof(header)) {
		Println("Invalid header size: %s", path);
		return sl_false;
	}
	if (ChaCha20FileEncryptor::changePassword(header, oldKey.getData(), (sl_uint32)(oldKey.getLength()), newKey.getData(), (sl_uint32)(newKey.getLength()))) {
		if (!(file->seekToBegin())) {
			Println("Failed to seek to begin: %s", path);
			return sl_false;
		}
		if (file->writeFully(header, sizeof(header)) != sizeof(header)) {
			Println("Failed to write new header: %s", path);
			return sl_false;
		}
		return sl_true;
	} else {
		Println("Invalid old Key on file: %s", path);
		return sl_false;
	}
}

sl_bool UpdateDirPassword(const String& oldKey, const String& newKey, const String& path)
{
	for (auto& fileName : File::getFiles(path)) {
		String pathFile = path + "/" + fileName;
		FileAttributes attrs = File::getAttributes(pathFile);
		if (!(attrs & FileAttributes::NotExist)) {
			if (attrs & FileAttributes::Directory) {
				UpdateDirPassword(oldKey, newKey, pathFile);
			} else {
				UpdateFilePassword(oldKey, newKey, pathFile);
			}
		}
	}
	return sl_true;
}

int main(int argc, const char * argv[])
{
	String method = argv[1];
	if (method == "e" || method == "d") {
		if (argc != 5) {
			PrintUsage();
			return -1;
		}
		sl_bool flagEncrypt = sl_false;
		if (method == "e") {
			flagEncrypt = sl_true;
		}
		String key = argv[2];
		String pathSrc = argv[3];
		String pathDst = argv[4];
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
	} else if (method == "c") {
		if (argc != 4) {
			PrintUsage();
			return -1;
		}
		String key = argv[2];
		String path = argv[3];
		CheckPassword(key, path);
	} else if (method == "u") {
		if (argc != 5) {
			PrintUsage();
			return -1;
		}
		String oldKey = argv[2];
		String newKey = argv[3];
		String path = argv[4];
		if (File::isDirectory(path)) {
			if (!(UpdateDirPassword(oldKey, newKey, path))) {
				return -1;
			}
		} else {
			if (!(UpdateFilePassword(oldKey, newKey, path))) {
				return -1;
			}
		}
	} else {
		PrintUsage();
		return -1;
	}
	return 0;
}
