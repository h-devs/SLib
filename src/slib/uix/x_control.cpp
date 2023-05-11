#include "slib/uix/control.h"

#include "slib/graphics/pen.h"

namespace slib
{

	XControlProvider::XControlProvider()
	{
		m_view = sl_null;
	}

	XControlProvider::~XControlProvider()
	{
	}

	void XControlProvider::initXControl(View* view)
	{
		m_view = view;
		view->setAntiAlias(sl_true, UIUpdateMode::Init);
		view->setBoundRadius(5.0f, UIUpdateMode::Init);
		view->setBorder(Pen::createSolidPen(1.0f, Color(0xd3, 0xd3, 0xd3)), UIUpdateMode::Init);
		view->setBorder(Pen::createSolidPen(1.0f, Color(0x1a, 0xc8, 0xaf)), ViewState::Focused, UIUpdateMode::Init);
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

}
