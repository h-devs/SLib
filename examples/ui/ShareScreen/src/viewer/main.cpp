#include <slib.h>

#include "../common.h"

using namespace slib;

int main(int argc, const char * argv[])
{
	Printf("Input the server address [localhost:%d]: ", DEFAULT_SERVER_PORT);
	String serverAddress = Console::readLine().trim();
	if (serverAddress.isEmpty()) {
		serverAddress = String::format("localhost:%d", DEFAULT_SERVER_PORT);
	}

	auto window = New<Window>();
	auto hlinear = New<HorizontalLinearLayout>();
	hlinear->setWidthFilling();
	hlinear->setHeightFilling();
	auto lvUsers = New<LabelList>();
	lvUsers->setWidthWeight(0.3f);
	lvUsers->setHeightFilling();
	hlinear->addChild(lvUsers);
	auto viewScreen = New<ImageView>();
	viewScreen->setWidthFilling();
	viewScreen->setHeightFilling();
	hlinear->addChild(viewScreen);
	window->addView(hlinear);

	window->setOnDestroy([](Window*) {
		UI::quitApp();
	});
	window->setMaximized(sl_true);
	window->setResizable(sl_true);
	window->setMaximizeButtonEnabled(sl_true);
	window->setMinimizeButtonEnabled(sl_true);
	window->setTitle("Browse Screens");
	window->show();

	lvUsers->setOnChangeSelection([viewScreen](ListBox*, UIEvent*) {
		viewScreen->setSource(sl_null);
	});

	auto timerList = Dispatch::setInterval([serverAddress, lvUsers](Timer*) {
		auto req = UrlRequest::sendSynchronous(HttpMethod::GET, String::format("http://%s/user_list", serverAddress));
		List<String> users;
		FromJson(req->getResponseContentAsJson(), users);
		lvUsers->setValues(users);
		lvUsers->setTitles(users);
		lvUsers->setItemCount(users.getCount());
	}, 1000);

	auto timerScreen = Dispatch::setInterval([serverAddress, lvUsers, viewScreen](Timer*) {
		auto req = UrlRequest::sendSynchronous(HttpMethod::GET, String::format("http://%s/screen/%s", serverAddress, lvUsers->getSelectedValue()));
		viewScreen->setSource(Image::loadFromMemory(req->getResponseContent()));
	}, 200);

	UI::runApp();

	return 0;
}
