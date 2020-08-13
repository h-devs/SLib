#include <slib.h>

using namespace slib;

sl_uint32 getRandom()
{
	sl_uint32 ret;
	Math::randomMemory(&ret, sizeof(ret));
	return ret;
}

int main(int argc, const char * argv[])
{
	String filePath = "D:\\Work\\SLIBIO\\SLib\\README.md";
	Memory memContent = File::readAllBytes(filePath);
	SLIB_ASSERT(memContent.isNotNull());

	MemoryReader readerMem(memContent);
	auto readerFile = BufferedSeekableReader::create(File::openForRead(filePath));
	SLIB_ASSERT(readerFile.isNotNull());

	sl_size sizeFile = memContent.getSize();
	static char buf[100000];
	static char buf2[100000];
	for (sl_uint32 i = 0; i < 1000; i++) {
		sl_size offset = getRandom() % sizeFile;
		sl_uint32 nIter = getRandom() % 50;
		SLIB_ASSERT(readerMem.seek(offset, SeekPosition::Begin));
		SLIB_ASSERT(readerFile->seek(offset, SeekPosition::Begin));
		for (sl_uint32 k = 0; k < nIter; k++) {
			sl_size size = getRandom() % sizeof(buf);
			sl_reg n = readerMem.readFully(buf, size);
			sl_reg n2 = readerFile->readFully(buf2, size);
			SLIB_ASSERT(n == n2);
			if (n > 0) {
				SLIB_ASSERT(Base::equalsMemory(buf, buf2, n));
			}
		}
	}

	Println("Test: OK!!!");

	return 0;
}
