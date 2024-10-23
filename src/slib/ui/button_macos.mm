/*
 *   Copyright (c) 2008-2018 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/definition.h"

#if defined(SLIB_UI_IS_MACOS)

#include "slib/ui/button.h"

#include "button_macos.h"

namespace slib
{

	namespace priv
	{

		SLIB_DEFINE_OBJECT(ButtonInstance, PlatformViewInstance)

		ButtonInstance::ButtonInstance()
		{
		}

		ButtonInstance::~ButtonInstance()
		{
		}

		static NSString* getButtonText(Button* view, const String& s)
		{
			if (view->isMnemonic()) {
				return Apple::getNSStringFromString(s.removeAll('&'));
			} else {
				return Apple::getNSStringFromString(s);
			}
		}

		void ButtonInstance::initialize(View* _view)
		{
			NSButton* handle = getHandle();
			Button* view = (Button*)_view;

			handle.bezelStyle = NSRoundedBezelStyle;
			handle.title = getButtonText(view, view->getText());
			if (view->isDefaultButton()) {
				[handle setKeyEquivalent:@"\r"];
			}
		}

		NSButton* ButtonInstance::getHandle()
		{
			return (NSButton*)m_handle;
		}

		void ButtonInstance::setText(Button* view, const String& text)
		{
			NSButton* handle = getHandle();
			if (handle != nil) {
				handle.title = getButtonText(view, text);
			}
		}

		void ButtonInstance::setDefaultButton(Button* view, sl_bool flag)
		{
			NSButton* handle = getHandle();
			if (handle != nil) {
				if (flag) {
					[handle setKeyEquivalent:@"\r"];
				} else {
					[handle setKeyEquivalent:@""];
				}
			}
		}

		sl_bool ButtonInstance::measureSize(Button* view, UISize& _out)
		{
			return UIPlatform::measureNativeWidgetFittingSize(this, _out);
		}

		sl_bool ButtonInstance::getChecked(CheckBox* view, sl_bool& _out)
		{
			NSButton* handle = getHandle();
			if (handle != nil) {
				_out = (handle.state == NSOnState ? sl_true : sl_false);
				return sl_true;
			}
			return sl_false;
		}

		void ButtonInstance::setChecked(CheckBox* view, sl_bool flag)
		{
			NSButton* handle = getHandle();
			if (handle != nil) {
				[handle setState: (flag ? NSOnState : NSOffState)];
			}
		}

	}

	using namespace priv;

	Ref<ViewInstance> Button::createNativeWidget(ViewInstance* parent)
	{
		return PlatformViewInstance::create<ButtonInstance, SLIBButtonHandle>(this, parent);
	}

	Ptr<IButtonInstance> Button::getButtonInstance()
	{
		return CastRef<ButtonInstance>(getViewInstance());
	}

}

using namespace slib;

@implementation SLIBButtonHandle

MACOS_VIEW_DEFINE_ON_CHILD_VIEW

-(id)initWithFrame:(NSRect)frame
{
	self = [super initWithFrame:frame];
	if (self != nil) {
		[self setAction: @selector(onClick)];
		[self setTarget:self];
	}
	return self;
}

-(void)onClick
{
	Ref<ButtonInstance> instance = m_viewInstance;
	if (instance.isNotNull()) {
		instance->onClick();
	}
}

- (BOOL)performKeyEquivalent:(NSEvent *)anEvent
{
	return NO;
}

@end

#endif
