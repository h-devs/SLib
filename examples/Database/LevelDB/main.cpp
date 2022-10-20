#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	String path = System::getHomeDirectory() + "/test";
	auto db = LevelDB::open(path);
	if (db.isNull()) {
		Println("Failed to open database!");
		return -1;
	}
	if (1) {
		sl_uint32 n = 0;
		for (sl_uint32 i = 0; i < 1000; i++) {
			//auto batch = db->createWriteBatch();
			for (sl_uint32 j = 0; j < 1000; j++) {
				String s = String::fromUint32(n);
				db->put(s, "I upgraded my MacBook Pro to Lion and one of the problems I've had is that Time Machine keeps running and running, and the fans go on.This usually seems to be in the state where it indexes the backup." + s);
				n++;
			}
			//db->commit();
		}
	}
	sl_uint32 a = 10000;
	for (sl_uint32 k = 0; k < 10000; k++) {
		Println("%s", db->get(String::fromUint32(a + k)));
	}
	Println("Finished!");
	while (1);
	return 0;
}
