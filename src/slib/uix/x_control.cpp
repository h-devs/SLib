#include "slib/uix/control.h"

#include "slib/graphics/pen.h"

namespace slib
{

	XControlProvider::XControlProvider()
	{
		m_view = sl_null;
		m_focusedBorder = Pen::createSolidPen(1.0f, Color(0x1a, 0xc8, 0xaf));
	}

	XControlProvider::~XControlProvider()
	{
	}

	void XControlProvider::initXControl(View* view)
	{
		m_view = view;
		view->setAntiAlias(sl_true, UIUpdateMode::Init);
		view->setBoundRadius(5.0f, UIUpdateMode::Init);
		view->setBorderColor(Color(0xd3, 0xd3, 0xd3), UIUpdateMode::Init);
		view->setBorderWidth(1, UIUpdateMode::Init);
	}

	Color XControlProvider::getFocusedBorderColor()
	{
		if (m_focusedBorder.isNotNull()) {
			return m_focusedBorder->getColor();
		}
		return Color::zero();
	}

	void XControlProvider::setFocusedBorderColor(const Color& color, UIUpdateMode mode)
	{
		m_focusedBorder = Pen::createSolidPen(1.0f, color);
		m_view->invalidate(mode);
	}

	void XControlProvider::drawXControlBorder(Canvas* canvas)
	{
		View* view = m_view;
		if (view->getFocusedView().isNotNull()) {
			view->drawBorder(canvas, m_focusedBorder);
		} else {
			view->drawBorder(canvas, view->getBorder());
		}
	}


	SLIB_DEFINE_OBJECT(XControl, ViewGroup)

	XControl::XControl()
	{
	}

	XControl::~XControl()
	{
	}

	void XControl::init()
	{
		ViewGroup::init();
		initXControl(this);
	}

	void XControl::onDrawBorder(Canvas* canvas)
	{
		drawXControlBorder(canvas);
	}

}
