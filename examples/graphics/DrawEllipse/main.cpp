#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	auto window = New<Window>();
	window->setClientWidth(600);
	window->setClientHeight(600);
	window->getContentView()->setOnDraw([](View*, Canvas* canvas) {
		canvas->drawEllipse(100, 100, 400, 400, Pen::createSolidPen(1, Color::Black));
	});
	window->setQuitOnDestroy();
	window->show();

	UI::runApp();

	return 0;
}