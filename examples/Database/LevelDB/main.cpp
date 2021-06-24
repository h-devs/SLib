#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	auto db = LevelDB::open(System::getHomeDirectory() + "/test");
	if (db.isNull()) {
		Println("Failed to open database!");
		return -1;
	}
	sl_uint32 n = 0;
	for (sl_uint32 i = 0; i < 100; i++) {
		auto batch = db->createWriteBatch();
		for (sl_uint32 j = 0; j < 100; j++) {
			String s = String::fromUint32(n);
			batch->put(s, "value" + s);
			n++;
		}
		batch->commit();
	}
	for (sl_uint32 k = 1001; k < 1100; k++) {
		Println("%s", db->get(String::fromUint32(k)));
	}
	return 0;
}
