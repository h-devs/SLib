#include <slib.h>

using namespace slib;

int main(int argc, const char * argv[])
{
	System::setDebugFlags();

	auto window = New<Window>();
	window->setWidth(1000);
	window->setHeight(600);

	auto view = New<View>();
	view->setWidthFilling();
	view->setHeightFilling();
	view->setBackgroundColor(Color::White);

	view->setOnDraw([](View*, Canvas* canvas) {
		auto penRed = Pen::createSolidPen(1.0f, Color::Red);
		auto penBlack = Pen::createSolidPen(1.0f, Color::Black);
		Ref<Font> font = Font::create(u8"Segoe Script", 100);
		const char32_t s[] = U"font example";
		sl_real x = 100.0f;
		sl_real y = 20.0f;
		canvas->drawLine(0.0f, y, 1000.0f, y, penBlack);
		canvas->drawLine(x, 0.0f, x, 1000.0f, penBlack);
		for (sl_size i = 0; i < CountOfArray(s) - 1; i++) {
			TextMetrics tm;
			if (font->measureChar(s[i], tm)) {
				tm.translate(x, y);
				canvas->drawRectangle(tm, penRed);
			}
			sl_real advance;
			auto path = font->getCharOutline(s[i], x, y, &advance);
			if (path) {
				canvas->fillPath(path, Color::Blue);
			}
			x += advance;
		}
	});
	window->addView(view);
	window->show();
	window->setQuitOnDestroy();
	UI::runApp();
	return 0;
}
