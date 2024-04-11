#include <slib.h>
#include <slib/db/redis.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	Ref<Redis> db = Redis::connect("172.20.31.200", 6379);
	if (db.isNull()) {
		Println("Cannot connect to the database!");
		return -1;
	}
	db->setLoggingErrors(sl_true);

	Println("PING: %s", db->execute("PING"));

	Println("SET: %s", db->put("foo", "hello world"));

	Println("GET foo: %s", db->get("foo"));

	Println("INCR counter: %s", db->incr("counter"));
	Println("INCR counter: %s", db->incr("counter"));

	Println("DEL: %s", db->remove("mylist"));
	for (int j = 0; j < 10; j++) {
		db->lpush("mylist", String::format("element-%d", j));
	}
	int i = 0;
	for (auto&& item : db->lrange("mylist")) {
		i++;
		Println("%d) %s", i, item);
	}

	return 0;
}
