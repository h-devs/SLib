/*
*   Copyright (c) 2008-2024 SLIBIO <https://github.com/SLIBIO>
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
			View,
			ViewInstance,
			ViewCell,
			ViewGroup,
			PlatformViewInstance,
			Window,
			WindowInstance,
			WindowContentView,
			Screen,
			UIEvent,
			UIEvent_End = UIEvent + 10,
			UIApp,
			MobileApp,
			MobileMainWindow,
			MobileGame,
			MobileGameView,
			Cursor,
			GestureDetector,
			GlobalEventMonitor,
			UIAnimation,
			UIAnimation_End = UIAnimation + 10,
			ViewLayout,
			WindowLayout,
			PageLayout,
			Menu,
			PlatformMenu,
			MenuItem,
			PlatformMenuItem,
			SystemTrayIcon,
			TextInput,
			UITextBox,
			UserNotification,
			PushNotificationClient,
			APNs,
			AudioView = Package_Ui + 500,
			Button,
			ButtonInstance,
			ButtonCell,
			CameraView,
			CheckBox,
			CheckBoxInstance,
			CheckBoxCell,
			ChromiumView,
			ChromiumViewInstance,
			CollectionView,
			ComboBox,
			ComboBoxInstance,
			ComboBoxCell,
			DatePicker,
			DatePickerInstance,
			DatePickerCell,
			Drawer,
			EditView,
			EditViewInstance,
			TextArea,
			TextAreaInstance,
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
			PickerView,
			PickerViewCell,
			PickerViewInstance,
			ProgressBar,
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
			TabView,
			TabViewInstance,
			TableLayout,
			GridView,
			GroupBox,
			TextView,
			TileLayout,
			TreeView,
			TreeViewItem,
			ViewAdapter,
			ViewListAdapter,
			ViewRowAdapter,
			VideoView,
			ViewPager,
			ViewPage,
			ViewPage_End = ViewPage + 5,
			ViewPageNavigationController,
			WebView,
			WebViewInstance,
			PdfView,
			MapView,
			MapView_End = MapView + 20
		};

	}
}

#endif
