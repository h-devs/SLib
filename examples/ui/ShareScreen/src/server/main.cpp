#include <slib.h>

#include "../common.h"

using namespace slib;

class UserInfo : public CRef
{
public:
	Memory screen;
	Time lastScreenTime;
};

CHashMap< String, Ref<UserInfo> > g_mapUsers;

int main(int argc, const char * argv[])
{
	Printf("Input the port number [%d]: ", DEFAULT_SERVER_PORT);
	sl_uint16 port = (sl_uint16)(Console::readLine().trim().parseUint32());
	if (!port) {
		port = DEFAULT_SERVER_PORT;
	}

	HttpServerParam param;

	param.port = port;

	param.router.GET("/screen/:userId", [](HttpServerContext* context) {
		auto userId = context->getParameter("userId");
		auto user = g_mapUsers.getValue(userId);
		if (user && user->screen && (Time::now() - user->lastScreenTime).getSecondCount() < 5) {
			context->setResponseContentType(ContentType::ImageJpeg);
			context->write(user->screen);
		} else {
			context->write("Not Found");
		}
		return true;
	});

	param.router.PUT("/screen/:userId", [](HttpServerContext* context) {
		auto userId = context->getParameter("userId");
		auto body = context->getRequestBody();
		if (!body) {
			return "Error";
		}
		auto user = g_mapUsers.getValue(userId);
		if (!user) {
			user = new UserInfo;
			g_mapUsers.put(userId, user);
		}
		user->screen = body;
		user->lastScreenTime = Time::now();
		return "OK";
	});

	param.router.GET("/user_list", [](HttpServerContext* context) {
		return Json(g_mapUsers.getAllKeys());
	});

	Ref<HttpServer> server = HttpServer::create(param);
	if (server.isNull()) {
		return -1;
	}

	Console::println("Server is running on port: %d", param.port);

	for (;;) {
		Console::println("\nPress x to exit!!!");
		if (Console::readChar() == 'x') {
			break;
		}
	}

	return 0;
}
