/*
*   Copyright (c) 2008-2021 SLIBIO <https://github.com/SLIBIO>
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

#ifndef CHECKHEADER_SLIB_UI_OBJECT_TYPES
#define CHECKHEADER_SLIB_UI_OBJECT_TYPES

#include "../object_types.h"

namespace slib
{
	namespace object_types
	{

		enum {
			Package_Ui = packages::Ui,
			UIEvent,
			AudioView,
			Button,
			ButtonInstance,
			ButtonCell,
			CameraView,
			ChatView,
			CheckBox,
			CheckBoxInstance,
			CheckBoxCell,
			ChromiumView,
			ChromiumViewInstance,
			CollectionView,
			ComboBox,
			ComboBoxInstance,
			ComboBoxCell,
			Cursor,
			DatePicker,
			DatePickerInstance,
			DatePickerCell,
			Drawer,
			EditView,
			EditViewInstance,
			TextArea,
			TextAreaInstance,
			GestureDetector,
			ImageView,
			LabelView,
			LabelViewCell,
			LabelList,
			LineView,
			LinearLayout,
			ListBox,
			ListControl,
			ListControlInstance,
			ListView,
			MobileApp,
			MobileMainWindow,
			MobileGame,
			MobileGameView,
			PickerView,
			PickerViewCell,
			PickerViewInstance,
			ProgressBar,
			QRCodeScanner,
			RadioButton,
			RadioButtonCell,
			RadioGroup,
			RefreshView,
			RenderView,
			RenderViewInstance,
			ScrollBar,
			ScrollView,
			ScrollViewInstance,
			SelectView,
			SelectViewInstance,
			SelectSwitch,
			SelectSwitchCell,
			Slider,
			SplitLayout,
			SwitchView,
			SystemTrayIcon,
			TabView,
			TabViewInstance,
			TableLayout,
			TableView,
			TextView,
			TileLayout,
			TreeView,
			TreeViewItem,
			ViewAdapter,
			ViewListAdapter,
			ViewRowAdapter,
			UIAnimationLoop,
			ViewTransformAnimationTarget,
			ViewTranslateAnimationTarget,
			ViewScaleAnimationTarget,
			ViewRotateAnimationTarget,
			ViewFrameAnimationTarget,
			ViewAlphaAnimationTarget,
			ViewBackgroundColorAnimationTarget,
			UIApp,
			Screen,
			KeyboardEvent,
			MouseEvent,
			MouseWheelEvent,
			TouchEvent,
			SetCursorEvent,
			DragEvent,
			Menu,
			MenuImpl,
			MenuItem,
			MenuItemImpl,
			FCM,
			XgPush,
			UserNotification,
			PushNotificationClient,
			APNs,
			WindowLayout,
			ViewLayout,
			PageLayout,
			TextInput,
			UITextBox,
			VideoView,
			View,
			ViewInstance,
			ViewCell,
			ViewGroup,
			Android_ViewInstance,
			Win32_ViewInstance,
			iOS_ViewInstance,
			macOS_ViewInstance,
			EFL_ViewInstance,
			GTK_ViewInstance,
			ViewPager,
			ViewPage,
			PopupBackground,
			ViewPageNavigationController,
			WebView,
			WebViewInstance,
			Window,
			WindowInstance,
			WindowContentView,
			PdfView,
			GroupBox
		};

	}
}

#endif
