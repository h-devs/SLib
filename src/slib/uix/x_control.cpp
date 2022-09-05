#include "slib/uix/control.h"

#include "slib/graphics/pen.h"

namespace slib
{

	SLIB_DEFINE_OBJECT(XControl, ViewGroup)

	XControl::XControl()
	{
		m_focusedBorder = Pen::createSolidPen(1.0f, Color(0x1a, 0xc8, 0xaf));
	}

	XControl::~XControl()
	{
	}

	void XControl::init()
	{
		ViewGroup::init();
		setAntiAlias(sl_true, UIUpdateMode::Init);
		setBoundRadius(5.0f, UIUpdateMode::Init);
		setBorderColor(Color(0xd3, 0xd3, 0xd3), UIUpdateMode::Init);
		setBorderWidth(1, UIUpdateMode::Init);
	}

	Color XControl::getFocusedBorderColor()
	{
		if (m_focusedBorder.isNotNull()) {
			return m_focusedBorder->getColor();
		}
		return Color::zero();
	}

	void XControl::setFocusedBorderColor(const Color& color, UIUpdateMode mode)
	{
		m_focusedBorder = Pen::createSolidPen(1.0f, color);
		invalidate(mode);
	}

	void XControl::onDrawBorder(Canvas* canvas)
	{
		if (getFocusedView().isNotNull()) {
			drawBorder(canvas, m_focusedBorder);
		} else {
			drawBorder(canvas, getBorder());
		}
	}

}
