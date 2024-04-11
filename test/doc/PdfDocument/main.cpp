#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	String dir = "E:\\Temp\\FileTest\\";
	auto files = File::getFiles(dir);
	for (auto&& file : files) {
		if (file.endsWith(".pdf")) {
			Println("------------ %s -----------", file);
			if (Pdf::isEncryptedFile(dir + file)) {
				Println("Encrypted");
			} else {
				Println("Not Encrypted");
			}
			Println(sl_null);
		}
	}
	return 0;
}
