#include "app.h"

SvgViewerApp::SvgViewerApp()
{
}

void SvgViewerApp::onStart()
{
	auto linear1 = New<LinearLayout>();
	auto linear2 = New<HorizontalLinearLayout>();
	
	auto btnOpen = New<Button>();
	btnOpen->setText("Open");
	btnOpen->setOnClick([this](View*) {
		onOpenSvg();
	});
	btnOpen->setCreatingNativeWidget();
	btnOpen->setWidthWrapping();
	btnOpen->setHeightWrapping();
	btnOpen->setPadding(3);
	linear2->addChild(btnOpen);

	auto btnBackground = New<Button>();
	btnBackground->setText("Background");
	btnBackground->setOnClick([this](View*) {
		onChangeBackground();
	});
	btnBackground->setCreatingNativeWidget();
	btnBackground->setWidthWrapping();
	btnBackground->setHeightWrapping();
	btnBackground->setMarginLeft(30);
	btnBackground->setPadding(3);
	linear2->addChild(btnBackground);

	linear2->setCenterHorizontal();
	linear2->setWidthWrapping();
	linear2->setHeightWrapping();
	linear2->setMargin(10);

	linear1->addChild(linear2);

	auto line = New<HorizontalLineView>();
	line->setWidthFilling();
	line->setHeightWrapping();
	line->setLineColor(Color::DarkGray);
	linear1->addChild(line);

	m_viewer = New<View>();
	m_viewer->setOnDraw([this](View*, Canvas* canvas) {
		onDrawSvg(canvas);
	});
	m_viewer->setWidthFilling();
	m_viewer->setHeightFilling();
	m_viewer->setBackgroundColor(Color::White);
	linear1->addChild(m_viewer);

	linear1->setWidthFilling();
	linear1->setHeightFilling();
	linear1->setFontSize(20);

	auto window = New<Window>();
	window->setTitle("SvgViewer");
	window->setFrame(100, 100, 600, 400);
	window->setResizable();
	window->addView(linear1);
	window->show();
	setMainWindow(window);
}

void SvgViewerApp::onOpenSvg()
{
	Ref<Svg> svg = Svg::loadFromFile(FileDialog::openFile(getMainWindow()));
	if (svg.isNotNull()) {
		m_svg = Move(svg);
		auto fWidth = m_svg->getDrawableWidth();
		auto fHeight = m_svg->getDrawableHeight();
		int width, height;
		if (fWidth > fHeight) {
			width = Math::clamp((int)fWidth, 300, 1000);
			height = (int)(fHeight / fWidth * width);
		} else {
			height = Math::clamp((int)fWidth, 200, 900);
			width = (int)(fWidth / fHeight * height);
			if (width < 300) {
				width = 300;
			}
		}
		getMainWindow()->setClientSize(width, height + getMainWindow()->getClientHeight() - m_viewer->getHeight());
		m_viewer->invalidate();
	}
}

void SvgViewerApp::onChangeBackground()
{
	if (m_viewer->getBackgroundColor() == Color::White) {
		m_viewer->setBackgroundColor(Color::LightGray);
	} else {
		m_viewer->setBackgroundColor(Color::White);
	}
}

void SvgViewerApp::onDrawSvg(Canvas* canvas)
{
	if (m_svg.isNotNull()) {
		canvas->setAntiAlias();
		Drawable::DrawParam param;
		m_svg->render(canvas, m_viewer->getBounds(), param);
	}
}
