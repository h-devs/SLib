#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	if (argc != 3) {
		Println("Usage: GenerateAppIcons <Source Image File> <Output Directory>");
		return -1;
	}
	String pathSrc = argv[1];
	String pathDst = argv[2];

	auto image = Image::loadFromFile(pathSrc);
	if (image.isNull()) {
		Println("Failed to open source image file: %s", pathSrc);
		return -1;
	}
	if (!(File::isDirectory(pathDst))) {
		Println("Output directory is invalid: %s", pathDst);
		return -1;
	}

	for (sl_uint32 i = 4; i <= 10; i++) {
		auto sub = image->stretch(1 << i, 1 << i);
		if (sub.isNull()) {
			Println("Unexpected image stretch error!");
			return -1;
		}
		String path = String::format("%s/%d.png", pathDst, 1 << i);
		if (!(sub->savePng(path))) {
			Println("Failed to write image file: %s", path);
			return -1;
		}
	}

	Println("Success");

	return 0;
}
