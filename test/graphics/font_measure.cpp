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
		{
			sl_real x = 100.0f;
			sl_real y = 20.0f;
			canvas->drawLine(0.0f, y, 1000.0f, y, penBlack);
			canvas->drawLine(x, 0.0f, x, 1000.0f, penBlack);
			Ref<Font> font = Font::create(u8"Segoe Script", 50);
			String str = "afgA\r\nTest\nFont Example";
			TextMetrics tm;
			if (font->measureText(str, sl_true, tm)) {
				tm.translate(x, y);
				canvas->fillRectangle(tm, Color::Green);
				canvas->drawText(str, x, y, font, Color::Red, Alignment::TopLeft, sl_true);
			}
		}
		{
			Ref<Font> font = Font::create(u8"Segoe Script", 150);
			sl_real x = 100.0f;
			sl_real y = 300.0f;
			canvas->drawLine(0.0f, y, 1000.0f, y, penBlack);
			canvas->drawLine(x, 0.0f, x, 1000.0f, penBlack);
			char32_t s[] = U"afgA";
			for (sl_size i = 0; i < CountOfArray(s) - 1; i++) {
				TextMetrics tm;
				if (font->measureChar(s[i], tm)) {
					canvas->drawRectangle(x, y, tm.advanceX, tm.advanceY, penBlack, Color(255, 255, 0, 30));
					canvas->drawText(String::create(s + i, 1), x, y, font, Color::Blue);
					tm.translate(x, y);
					canvas->drawRectangle(tm, penRed);
					x += tm.advanceX;
				}
			}
		}
	});
	window->addView(view);
	window->show();
	window->setQuitOnDestroy();
	UI::runApp();
	return 0;
}
