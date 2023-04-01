/*
 *   Copyright (c) 2008-2023 SLIBIO <https://github.com/SLIBIO>
 *
 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:
 *
 *   The above copyright notice and this permission notice shall be included in
 *   all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *   THE SOFTWARE.
 */

#include "slib/ui/select_view.h"

#include "slib/graphics/canvas.h"
#include "slib/core/safe_static.h"

#include "label_list_base_impl.h"

#if defined(SLIB_UI_IS_MACOS) || defined(SLIB_UI_IS_IOS) || defined(SLIB_UI_IS_WIN32) || defined(SLIB_UI_IS_ANDROID) || defined(SLIB_UI_IS_GTK)
#	define HAS_NATIVE_WIDGET_IMPL 1
#else
#	define HAS_NATIVE_WIDGET_IMPL 0
#endif

namespace slib
{

	SLIB_DEFINE_OBJECT(SelectView, View)
	SLIB_DEFINE_SINGLE_SELECTION_VIEW_INSTANCE_NOTIFY_FUNCTIONS(SelectView, sl_uint32, ISelectViewInstance, getSelectViewInstance)

	SelectView::SelectView()
	{
		setSupportedNativeWidget(HAS_NATIVE_WIDGET_IMPL);
		setCreatingNativeWidget(HAS_NATIVE_WIDGET_IMPL);

		setUsingFont(sl_true);
		setBorder(sl_true, UIUpdateMode::Init);
		setBackgroundColor(Color::White, UIUpdateMode::Init);
		setSavingCanvasState(sl_false);
#if !defined(SLIB_PLATFORM_IS_MOBILE)
		setFocusable(sl_true);
#endif

		m_gravity = Alignment::Left;
		m_textColor = Color::Black;
	}

	SelectView::~SelectView()
	{
	}

	Alignment SelectView::getGravity()
	{
		return m_gravity;
	}

	void SelectView::setGravity(const Alignment& gravity, UIUpdateMode mode)
	{
		Ptr<ISelectViewInstance> instance = getSelectViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setGravity, gravity, mode)
		}
		m_gravity = gravity;
		if (m_cell.isNotNull()) {
			m_cell->gravity = gravity;
		}
		if (instance.isNotNull()) {
			instance->setGravity(this, gravity);
		} else {
			invalidate(mode);
		}
	}

	Color SelectView::getTextColor()
	{
		return m_textColor;
	}

	void SelectView::setTextColor(const Color& color, UIUpdateMode mode)
	{
		Ptr<ISelectViewInstance> instance = getSelectViewInstance();
		if (instance.isNotNull()) {
			SLIB_VIEW_RUN_ON_UI_THREAD(setTextColor, color, mode)
		}
		m_textColor = color;
		if (m_cell.isNotNull()) {
			m_cell->textColor = color;
		}
		if (instance.isNotNull()) {
			instance->setTextColor(this, color);
		} else {
			invalidate(mode);
		}
	}

	void SelectView::_initCell()
	{
		if (m_cell.isNull()) {
			Ref<SelectSwitchCell> cell = new SelectSwitchCell;
			if (cell.isNotNull()) {
				cell->setView(this, sl_true);
				cell->initLabelList(this);
				cell->gravity = m_gravity;
				cell->textColor = m_textColor;
				cell->onSelectItem = SLIB_FUNCTION_WEAKREF(this, _onSelectItem);
				m_cell = cell;
			}
		}
	}

	void SelectView::onDraw(Canvas* canvas)
	{
		_initCell();
		if (m_cell.isNotNull()) {
			m_cell->onDraw(canvas);
		}
	}

	void SelectView::onMouseEvent(UIEvent* ev)
	{
		if (m_cell.isNotNull()) {
			m_cell->onMouseEvent(ev);
		}
	}

	void SelectView::onUpdateLayout()
	{
		sl_bool flagHorizontalWrapping = isWidthWrapping();
		sl_bool flagVerticalWrapping = isHeightWrapping();

		if (!flagVerticalWrapping && !flagHorizontalWrapping) {
			return;
		}

		Ptr<ISelectViewInstance> instance = getSelectViewInstance();
		if (instance.isNotNull()) {
			UISize size;
			if (instance->measureSize(this, size)) {
				if (flagHorizontalWrapping) {
					setLayoutWidth(size.x);
				}
				if (flagVerticalWrapping) {
					setLayoutHeight(size.y);
				}
				return;
			}
		}

		if (m_cell.isNotNull()) {
			updateLayoutByViewCell(m_cell.get());
		} else {
			Ref<Font> font = getFont();
			if (font.isNull()) {
				return;
			}
			if (flagHorizontalWrapping) {
				setLayoutWidth((sl_ui_len)(font->getFontHeight() * 4));
			}
			if (flagVerticalWrapping) {
				setLayoutHeight((sl_ui_len)(font->getFontHeight() * 1.5f));
			}
		}
	}

	SLIB_DEFINE_EVENT_HANDLER(SelectView, SelectItem, sl_uint32 index, sl_uint32 former, UIEvent* ev)

	void SelectView::dispatchSelectItem(sl_uint32 index, sl_uint32 former, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(SelectItem, index, former, ev)
	}

	void SelectView::_onSelectItem(sl_uint32 index, UIEvent* ev)
	{
		notifySelectItem(index, ev, UIUpdateMode::Redraw);
	}

	void SelectView::_onSelectItem_NW(sl_uint32 index)
	{
		Ref<UIEvent> ev = UIEvent::createUnknown(Time::now());
		if (ev.isNotNull()) {
			notifySelectItem(index, ev.get(), UIUpdateMode::None);
		}
	}

#if !HAS_NATIVE_WIDGET_IMPL
	Ref<ViewInstance> SelectView::createNativeWidget(ViewInstance* parent)
	{
		return sl_null;
	}

	Ptr<ISelectViewInstance> SelectView::getSelectViewInstance()
	{
		return sl_null;
	}
#endif

	void ISelectViewInstance::setGravity(SelectView* view, const Alignment& gravity)
	{
	}

	void ISelectViewInstance::setTextColor(SelectView* view, const Color& color)
	{
	}

	sl_bool ISelectViewInstance::measureSize(SelectView* view, UISize& _out)
	{
		return sl_false;
	}


	SLIB_DEFINE_OBJECT(SelectSwitch, View)
	SLIB_DEFINE_SINGLE_SELECTION_VIEW_NOTIFY_FUNCTIONS(SelectSwitch, sl_uint32)

	SelectSwitch::SelectSwitch()
	{
		setUsingFont(sl_true);
		setBorder(sl_true, UIUpdateMode::Init);
		setBackgroundColor(Color::White, UIUpdateMode::Init);
		setSavingCanvasState(sl_false);
#if !defined(SLIB_PLATFORM_IS_MOBILE)
		setFocusable(sl_true);
#endif

		m_cell = new SelectSwitchCell;
	}

	SelectSwitch::~SelectSwitch()
	{
	}

	void SelectSwitch::init()
	{
		View::init();

		m_cell->setView(this, sl_true);
		m_cell->initLabelList(this);
		m_cell->onSelectItem = SLIB_FUNCTION_WEAKREF(this, _onSelectItem);
	}

	const UISize& SelectSwitch::getIconSize()
	{
		return m_cell->iconSize;
	}

	void SelectSwitch::setIconSize(const UISize& size, UIUpdateMode mode)
	{
		m_cell->iconSize = size;
		invalidateLayoutOfWrappingControl(mode);
	}

	void SelectSwitch::setIconSize(sl_ui_len width, sl_ui_len height, UIUpdateMode mode)
	{
		setIconSize(UISize(width, height), mode);
	}

	void SelectSwitch::setIconSize(sl_ui_len size, UIUpdateMode mode)
	{
		setIconSize(UISize(size, size), mode);
	}

	sl_ui_len SelectSwitch::getIconWidth()
	{
		return m_cell->iconSize.x;
	}

	void SelectSwitch::setIconWidth(sl_ui_len width, UIUpdateMode mode)
	{
		setIconSize(UISize(width, m_cell->iconSize.y), mode);
	}

	sl_ui_len SelectSwitch::getIconHeight()
	{
		return m_cell->iconSize.y;
	}

	void SelectSwitch::setIconHeight(sl_ui_len height, UIUpdateMode mode)
	{
		setIconSize(UISize(m_cell->iconSize.x, height), mode);
	}

	Ref<Drawable> SelectSwitch::getLeftIcon()
	{
		return m_cell->leftIcon;
	}

	void SelectSwitch::setLeftIcon(const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		m_cell->leftIcon = icon;
		invalidate(mode);
	}

	Ref<Drawable> SelectSwitch::getRightIcon()
	{
		return m_cell->rightIcon;
	}

	void SelectSwitch::setRightIcon(const Ref<Drawable>& icon, UIUpdateMode mode)
	{
		m_cell->rightIcon = icon;
		invalidate(mode);
	}

	Alignment SelectSwitch::getGravity()
	{
		return m_cell->gravity;
	}

	void SelectSwitch::setGravity(const Alignment& gravity, UIUpdateMode mode)
	{
		m_cell->gravity = gravity;
		invalidate(mode);
	}

	Color SelectSwitch::getTextColor()
	{
		return m_cell->textColor;
	}

	void SelectSwitch::setTextColor(const Color& color, UIUpdateMode mode)
	{
		m_cell->textColor = color;
		invalidate(mode);
	}

	void SelectSwitch::onDraw(Canvas* canvas)
	{
		m_cell->onDraw(canvas);
	}

	void SelectSwitch::onMouseEvent(UIEvent* ev)
	{
		m_cell->onMouseEvent(ev);
	}

	void SelectSwitch::onUpdateLayout()
	{
		updateLayoutByViewCell(m_cell.get());
	}

	SLIB_DEFINE_EVENT_HANDLER(SelectSwitch, SelectItem, sl_uint32 index, sl_uint32 former, UIEvent* ev)

	void SelectSwitch::dispatchSelectItem(sl_uint32 index, sl_uint32 former, UIEvent* ev)
	{
		SLIB_INVOKE_EVENT_HANDLER(SelectItem, index, former, ev)
	}

	void SelectSwitch::_onSelectItem(sl_uint32 index, UIEvent* ev)
	{
		notifySelectItem(index, ev, UIUpdateMode::Redraw);
	}

	namespace {

		enum
		{
			ICON_NONE = 0,
			ICON_LEFT = 1,
			ICON_RIGHT = 2,
			ICON_DOWN = 3
		};

		class DefaultIcon : public Drawable
		{
		public:
			Ref<Brush> m_brush;
			Point m_pts[3];

		public:
			DefaultIcon(int type)
			{
				m_brush = Brush::createSolidBrush(Color::Black);
				if (type == ICON_LEFT) {
					m_pts[0] = Point(0.67f, 0.24f);
					m_pts[1] = Point(0.33f, 0.51f);
					m_pts[2] = Point(0.67f, 0.78f);
				} else if (type == ICON_RIGHT) {
					m_pts[0] = Point(0.33f, 0.24f);
					m_pts[1] = Point(0.67f, 0.51f);
					m_pts[2] = Point(0.33f, 0.78f);
				} else if (type == ICON_DOWN) {
					m_pts[0] = Point(0.3f, 0.35f);
					m_pts[1] = Point(0.5f, 0.65f);
					m_pts[2] = Point(0.7f, 0.35f);
				}
			}

		public:
			sl_real getDrawableWidth() override
			{
				return 1;
			}

			sl_real getDrawableHeight() override
			{
				return 1;
			}

			void onDrawAll(Canvas* canvas, const Rectangle& rectDst, const DrawParam& param) override
			{
				if (m_brush.isNotNull()) {
					Point pts[3];
					for (int i = 0; i < 3; i++) {
						pts[i].x = rectDst.left + rectDst.getWidth() * m_pts[i].x;
						pts[i].y = rectDst.top + rectDst.getHeight() * m_pts[i].y;
					}
					canvas->fillPolygon(pts, 3, m_brush);
				}
			}

		};

		class DefaultResources
		{
		public:
			Ref<Drawable> leftIcon;
			Ref<Drawable> rightIcon;
			Ref<Drawable> downIcon;

		public:
			DefaultResources()
			{
				leftIcon = new DefaultIcon(ICON_LEFT);
				rightIcon = new DefaultIcon(ICON_RIGHT);
				downIcon = new DefaultIcon(ICON_DOWN);
			}

		};

		SLIB_SAFE_STATIC_GETTER(DefaultResources, GetDefaultResources)

	}

	SLIB_DEFINE_OBJECT(SelectSwitchCell, SingleSelectionViewCellBase<sl_uint32>)

	SelectSwitchCell::SelectSwitchCell()
	{
		gravity = Alignment::Left;
		textColor = Color::Black;

		iconSize.x = 0;
		iconSize.y = 0;

		DefaultResources* def = GetDefaultResources();
		if (def) {
			leftIcon = def->leftIcon;
			rightIcon = def->rightIcon;
		}

		m_clickedIconNo = ICON_NONE;
	}

	SelectSwitchCell::~SelectSwitchCell()
	{
	}

	void SelectSwitchCell::onDraw(Canvas* canvas)
	{
		canvas->drawText(titleGetter(selectedIndex), getFrame(), getFont(), textColor, Alignment::MiddleCenter);
		canvas->draw(getLeftIconRegion(), leftIcon);
		canvas->draw(getRightIconRegion(), rightIcon);
	}

	void SelectSwitchCell::onMouseEvent(UIEvent* ev)
	{
		UIAction action = ev->getAction();
		UIPoint pt = ev->getPoint();
		if (action == UIAction::LeftButtonDown || action == UIAction::TouchBegin) {
			if (getLeftIconRegion().containsPoint(pt)) {
				m_clickedIconNo = ICON_LEFT;
				ev->stopPropagation();
			} else if (getRightIconRegion().containsPoint(pt)) {
				m_clickedIconNo = ICON_RIGHT;
				ev->stopPropagation();
			}
		} else if (action == UIAction::MouseLeave || action == UIAction::TouchCancel) {
			m_clickedIconNo = ICON_NONE;
		} else if (action == UIAction::LeftButtonUp || action == UIAction::TouchEnd) {
			if (m_clickedIconNo == ICON_LEFT) {
				if (getLeftIconRegion().containsPoint(pt)) {
					sl_uint32 index = selectedIndex;
					if (index > 0) {
						index--;
						selectedIndex = index;
						onSelectItem(index, ev);
						invalidate();
					}
				}
			} else if (m_clickedIconNo == ICON_RIGHT) {
				if (getRightIconRegion().containsPoint(pt)) {
					sl_uint32 index = selectedIndex;
					if (index + 1 < itemCount) {
						index++;
						selectedIndex = index;
						onSelectItem(index, ev);
						invalidate();
					}
				}
			}
			m_clickedIconNo = ICON_NONE;
		}
	}

	void SelectSwitchCell::onMeasure(UISize& size, sl_bool flagHorizontalWrapping, sl_bool flagVerticalWrapping)
	{
		Ref<Font> font = getFont();
		if (flagHorizontalWrapping) {
			sl_ui_pos width = iconSize.x * 2;
			if (font.isNotNull()) {
				sl_ui_pos t = (sl_ui_pos)(font->getFontHeight());
				if (t > 0) {
					width += t * 4;
				}
			}
			if (width < 0) {
				width = 0;
			}
			size.x = width;
		}
		if (flagVerticalWrapping) {
			sl_ui_pos height = 0;
			if (font.isNotNull()) {
				height = (sl_ui_pos)(font->getFontHeight() * 1.5f);
				if (height < 0) {
					height = 0;
				}
			}
			if (height < iconSize.y) {
				height = iconSize.y;
			}
			size.y = height;
		}
	}

	UIRect SelectSwitchCell::getLeftIconRegion()
	{
		UIRect frame = getFrame();
		sl_ui_pos heightView = frame.getHeight();
		if (heightView < 0) {
			heightView = 0;
		}
		sl_ui_pos heightIcon;
		if (iconSize.y > 0) {
			heightIcon = iconSize.y;
		} else {
			heightIcon = heightView;
		}
		if (iconSize.x > 0) {
			frame.right = frame.left + iconSize.x;
		} else {
			frame.right = frame.left + heightIcon;
		}
		frame.top += (heightView - heightIcon) / 2;
		frame.bottom = frame.top + heightIcon;
		frame.fixSizeError();
		return frame;
	}

	UIRect SelectSwitchCell::getRightIconRegion()
	{
		UIRect frame = getFrame();
		sl_ui_pos heightView = frame.getHeight();
		if (heightView < 0) {
			heightView = 0;
		}
		sl_ui_pos heightIcon;
		if (iconSize.y > 0) {
			heightIcon = iconSize.y;
		} else {
			heightIcon = heightView;
		}
		if (iconSize.x > 0) {
			frame.left = frame.right - iconSize.x;
		} else {
			frame.left = frame.right - heightIcon;
		}
		frame.top += (heightView - heightIcon) / 2;
		frame.bottom = frame.top + heightIcon;
		frame.fixSizeError();
		return frame;
	}

}
