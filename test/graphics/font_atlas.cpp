﻿#include <slib.h>
#include <slib/graphics/font_atlas.h>

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
		sl_real x = 100.0f;
		canvas->drawLine(x, 0.0f, x, 1000.0f, penBlack);
		{
			sl_real y = 20.0f;
			canvas->drawLine(0.0f, y, 1000.0f, y, penBlack);
			Ref<Font> font = Font::create(u8"Segoe Script", 50);
			Ref<FontAtlas> atlas = font->getAtlas();
			String str = "afgA\r\nTest\nfont example";
			TextMetrics tm;
			if (atlas->measureText(str, sl_true, tm)) {
				tm.translate(x, y);
				canvas->fillRectangle(tm, Color::Green);
				Canvas::DrawTextParam param;
				param.x = x;
				param.y = y;
				param.atlas = atlas;
				param.color = Color::Red;
				param.text = str;
				param.flagMultiLine = sl_true;
				canvas->drawText(param);
				canvas->drawRectangle(x, y, tm.advanceX, tm.advanceY, penRed);
			}
		}
		{
			Ref<Font> font = Font::create(u8"Segoe Script", 150);
			Ref<FontAtlas> atlas = font->getAtlas()->createStroker(10);
			sl_real y = 300.0f;
			canvas->drawLine(0.0f, y, 1000.0f, y, penBlack);
			char32_t s[] = U"afgAe";
			for (sl_size i = 0; i < CountOfArray(s) - 1; i++) {
				FontAtlasChar fac;
				if (atlas->getChar(s[i], fac)) {
					canvas->drawRectangle(x, y, fac.metrics.advanceX, fac.metrics.advanceY, penBlack, Color(255, 255, 0, 30));
					Canvas::DrawParam param;
					param.colorMatrix.setOverlay(Color::Blue);
					param.useColorMatrix = sl_true;
					fac.metrics.translate(x, y);
					canvas->draw(fac.metrics, fac.bitmap, fac.region, param);
					canvas->drawRectangle(fac.metrics, penRed);
					x += fac.metrics.advanceX;
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