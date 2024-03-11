#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	String dir = "E:\\Temp\\FileTest\\";
	auto files = File::getFiles(dir);
	for (auto&& file : files) {
		if (file.endsWith(".rar")) {
			Println("------------ %s -----------", file);
			if (RarFile::isEncryptedFile(dir + file)) {
				Println("Encrypted");
			} else {
				Println("%s", Json(RarFile::getFileNamesInFile(dir + file)));
			}
			Println(sl_null);
		}
	}
	return 0;
}
