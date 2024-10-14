#include <slib.h>
#include <slib/graphics/freetype.h>

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
		Ref<FreeType> font = FreeType::loadSystemFont(u8"Segoe Script");
		font->setSize(100);
		sl_real x = 100.0f;
		canvas->drawLine(x, 0.0f, x, 1000.0f, penBlack);
		{
			sl_real y = 20.0f;
			canvas->drawLine(0.0f, y, 1000.0f, y, penBlack);
			String str = "font example";
			TextMetrics tm;
			if (font->measureText(str, tm)) {
				Rectangle rc(x + tm.left, y + tm.top, x + tm.right, y + tm.bottom);
				canvas->fillRectangle(rc, Color::Green);
				auto image = Image::create((sl_uint32)(tm.getWidth()), (sl_uint32)(tm.getHeight()));
				image->resetPixels(Color::zero());
				font->drawText(image, -tm.left, -tm.top, str, Color::Red);
				canvas->draw(rc, image);
				canvas->drawRectangle(x, y, tm.advanceX, tm.advanceY, penRed);
			}
		}
		{
			sl_real y = 200.0f;
			canvas->drawLine(0.0f, y, 1000.0f, y, penBlack);
			String str = "font example";
			TextMetrics tm;
			if (font->measureText(str, tm)) {
				sl_real w = 2;
				Rectangle rc(x + tm.left - w, y + tm.top - w, x + tm.right + w, y + tm.bottom + w);
				canvas->fillRectangle(rc, Color::Green);
				auto image = Image::create((sl_uint32)(tm.getWidth() + w * 2), (sl_uint32)(tm.getHeight() + w * 2));
				image->resetPixels(Color::zero());
				font->strokeText(image, w - tm.left, w - tm.top, str, Color::Red, w * 2);
				canvas->draw(rc, image);
				canvas->drawRectangle(x, y, tm.advanceX, tm.advanceY, penRed);
			}
		}
		{
			sl_real y = 380.0f;
			canvas->drawLine(0.0f, y, 1000.0f, y, penBlack);
			char32_t s[] = U"font example";
			TextMetrics tm;
			if (font->measureText(StringView32(s, CountOfArray(s) - 1), tm)) {
				canvas->fillRectangle(x + tm.left, y + tm.top, tm.getWidth(), tm.getHeight(), Color::Green);
				canvas->drawRectangle(x, y, tm.advanceX, tm.advanceY, penRed);
				for (sl_size i = 0; i < CountOfArray(s) - 1; i++) {
					sl_real advance;
					auto path = font->getCharOutline(s[i], x, y, &advance);
					if (path) {
						canvas->drawPath(path, penRed);
					}
					x += advance;
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
