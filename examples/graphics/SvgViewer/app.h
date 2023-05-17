#pragma once

#include <slib.h>
#include <slib/graphics/svg.h>

using namespace slib;

class SvgViewerApp : public UIApp
{
	SLIB_APPLICATION(SvgViewerApp)
public:
	SvgViewerApp();

protected:
	void onStart() override;

private:
	void onOpenSvg();

	void onChangeBackground();

	void onDrawSvg(Canvas* canvas);

private:
	Ref<Svg> m_svg;
	Ref<View> m_viewer;

};
