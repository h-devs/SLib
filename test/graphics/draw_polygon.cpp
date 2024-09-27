#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	auto window = New<Window>();
	window->setWidth(800);
	window->setHeight(500);

	auto view = New<View>();
	view->setWidthFilling();
	view->setHeightFilling();
	view->setBackgroundColor(Color::White);
	auto points = List<Point>::create();
	view->setOnMouseEvent([points](View* view, UIEvent* ev) {
		auto action = ev->getAction();
		if (action == UIAction::LeftButtonDown) {
			points.ref->add(ev->getPoint());
			view->invalidate();
		} else if (action == UIAction::RightButtonDown) {
			points.removeAll();
			view->invalidate();
		}
	});
	view->setOnDraw([points](View*, Canvas* canvas) {
		auto penGreen = Pen::createSolidPen(10, Color::Green);
		canvas->drawPolygon(points, penGreen, Color::Red);
		for (auto&& pt : points) {
			canvas->fillEllipse(pt.x - 4, pt.y - 4, 8, 8, Color::Blue);
		}
	});
	window->addView(view);
	window->show();
	window->setQuitOnDestroy();
	UI::runApp();
	return 0;
}
