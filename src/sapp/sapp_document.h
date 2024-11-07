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

#ifndef CHECKHEADER_SLIB_SDEV_SAPP_DOCUMENT
#define CHECKHEADER_SLIB_SDEV_SAPP_DOCUMENT

#include "sapp_values.h"
#include "sapp_resources.h"

#include "slib/core/map.h"
#include "slib/core/locale.h"

namespace slib
{

	class SAppModuleConfiguration
	{
	public:
		String app_path;
		List<SAppModuleConfiguration> includes;
	};

	class SAppConfiguration : public SAppModuleConfiguration
	{
	public:
		struct GenerateCppConfig
		{
			String target_path;
			String ns;

			struct TypeConfig
			{
				sl_bool map = sl_true;
				struct FilterConfig
				{
					Nullable<sl_bool> layout;
					List<String> include;
					List<String> exclude;
				} filter;
			};
			TypeConfig string;
			TypeConfig color;
			TypeConfig drawable;
			TypeConfig menu;

			struct RawConfig : TypeConfig
			{
				List<String> compress;
			} raw;

			struct LayoutConfig
			{
				List<String> include_headers;
				List<String> include_headers_in_cpp;
			} layout;

		} generate_cpp;

		struct SimulatorConfig
		{
			Locale locale = Locale::Unknown;
		} simulator;

		struct LocaleConfig
		{
			List<Locale> exclude;
		} locale;
	};

	class SAppLayoutSimulationWindow;

	class SAppSimulateLayoutParam
	{
	public:
		UISize pageSize;
		Function<void(SAppLayoutSimulationWindow*)> onClosePage;
		Function<void(SAppLayoutSimulationWindow*)> onCloseWindow;

	public:
		SAppSimulateLayoutParam();
	};

	class SAppDocument : public Object
	{
	public:
		SAppDocument();
		~SAppDocument();

	public:
		sl_bool open(const String& filePath);

		void close();

		sl_bool isOpened();


		void clearAllResources();

		sl_bool openResources();

		sl_bool openUiResource(const String& filePath);


		sl_bool generateCpp();

		List< Ref<SAppLayoutResource> > getLayouts();

		sl_bool simulateLayoutInWindow(const String& layoutName, SAppSimulateLayoutParam& param);

		Locale getCurrentSimulatorLocale();

		// Raw Resource
		sl_bool openRawResources(const String& pathResources);

		sl_bool generateCppForRawResources(const String& _namespace, const String& pathOutput);


		// Utilities
		static String getShortcutKeyDefinitionString(const KeycodeAndModifiers& km, sl_bool flagMac);

		static String getGlobalName(const String& fileNamespace, const String& name);

		static String getGlobalStyleName(const String& fileNamespace, const String& theme, const String& name);

		String resolvePath(const String& path, const String& currentFilePath);

	public:
		static void log(const StringView& text);

		template <class... ARGS>
		static void log(const StringView& fmt, ARGS&&... args)
		{
			log(String::format(fmt, Forward<ARGS>(args)...));
		}

		static void logError(const StringView& text);

		template <class... ARGS>
		static void logError(const StringView& fmt, ARGS&&... args)
		{
			logError(String::format(fmt, Forward<ARGS>(args)...));
		}

		static void logErrorSource(const StringView& filePath, sl_size line, sl_size col, const StringView& text);

		static void logError(const Ref<XmlElement>& element, const StringView& text);

		template <class... ARGS>
		static void logError(const Ref<XmlElement>& element, const StringView& fmt, ARGS&&... args)
		{
			logError(element, String::format(fmt, Forward<ARGS>(args)...));
		}

	protected:
		sl_bool _openResourcesExceptUi();
		sl_bool _openResourcesExceptUi(const SAppModuleConfiguration& conf, HashSet<String>& includedSet);
		sl_bool _openResourcesExceptUi(const String& pathApp);
		sl_bool _openImageResources(const String& pathApp);
		sl_bool _openRawResources(const String& pathApp);
		sl_bool _openGlobalResources(const String& pathApp, const String& subdir, sl_bool flagLoadHierarchically);
		sl_bool _openUiResources();
		sl_bool _openUiResources(const SAppModuleConfiguration& conf, HashSet<String>& includedSet);
		sl_bool _openUiResources(const String& pathLayouts);
		sl_bool _openUiResource(const String& path);
		sl_bool _openUiResourceByName(const String& name);
		sl_bool _openUiResourceByName(const String& name, sl_bool& flagFound, const SAppModuleConfiguration& conf, HashSet<String>& includedSet);
		sl_bool _isExcludedLocale(const Locale& locale);

		// Resources Entry
		sl_bool _parseModuleConfiguration(const String& filePath, SAppModuleConfiguration& conf, const Function<sl_bool(XmlElement* root)>& onAdditionalConfig);
		sl_bool _parseConfiguration(const String& filePath, SAppConfiguration& conf);
		void _freeResources();
		sl_bool _parseResourcesXml(const String& fileNamespace, const String& filePath);
		sl_bool _generateResourcesH(const String& targetPath);

		// Raw Resources
		sl_bool _registerRawResources(const String& fileDirPath);
		sl_bool _registerRawResources(SAppRawResource* parent, const String& resourcePath, const String& fileDirPath);
		sl_bool _registerRawResource(const String& resourceName, const String& resourcePath, const String& filePath, String& outName, SAppRawResource* parent, const String& drawableName);
		sl_bool _generateRawCpp(const String& targetPath, const String& namespace1, const String& namespace2);
		sl_bool _generateRawCppItem(SAppRawResource* item, const String& targetPath, const String& relativePath, const String& namespacePath, StringBuffer& header, StringBuffer& cpp, StringBuffer& map, StringBuffer& data, sl_uint32 tabCountStart, sl_uint32 tabCountRelative);
		sl_bool _generateRawDataFile(const String& targetPath, const String& sourcePath, const String& resourceName, sl_bool flagCompress);

		// String Resources
		sl_bool _parseStringResources(const String& fileNamespace, const Ref<XmlElement>& element, const Locale& localeDefault, const String16& source);
		sl_bool _parseStringResource(const String& fileNamespace, const Ref<XmlElement>& element, const Locale& localeDefault, sl_bool flagVariants, const String16& source);
		Ref<SAppStringResource> _registerOrGetStringResource(const String& name, const Ref<XmlElement>& element);
		sl_bool _registerStringResourceItem(SAppStringResourceItem& item, const Ref<XmlElement>& element, const Locale& locale, const String16& source);
		sl_bool _generateStringsCpp(const String& targetPath);
		void _generateStringsCpp_Item(StringBuffer& sbCpp, const String& resourceName, const String& varName, const SAppStringResourceItem& item);
		sl_bool _getStringAccessString(const String& fileNamespace, const SAppStringValue& value, String& result);
		sl_bool _getStringDataAccessString(const String& fileNamespace, const SAppStringValue& value, String& result);
		sl_bool _getStringValue(const String& fileNamespace, const SAppStringValue& value, String& result);
		sl_bool _checkStringValue(const String& fileNamespace, const SAppStringValue& value);
		sl_bool _checkStringResource(const String& fileNamespace, const SAppStringValue& value, String* outName = sl_null, Ref<SAppStringResource>* outResource = sl_null, SAppStringResourceItem* outItem = sl_null);

		// Color Resources
		sl_bool _parseColorResource(const String& fileNamespace, const Ref<XmlElement>& element);
		sl_bool _generateColorsCpp(const String& targetPath);
		sl_bool _getColorAccessString(const String& fileNamespace, const SAppColorValue& value, String& result);
		sl_bool _getColorDataAccessString(const String& fileNamespace, const SAppColorValue& value, String& result);
		sl_bool _getColorValue(const String& fileNamespace, const SAppColorValue& value, Color& result);
		sl_bool _checkColorValue(const String& fileNamespace, const SAppColorValue& value);
		sl_bool _checkColorName(const String& fileNamespace, const String& name, const Ref<XmlElement>& element, String* outName = sl_null, Ref<SAppColorResource>* outResource = sl_null);

		// Drawable Resources
		sl_bool _generateDrawablesCpp(const String& targetPath);
		sl_bool _getDrawableAccessString(const String& fileNamespace, const SAppDrawableValue& value, String& result);
		sl_bool _getDrawableDataAccessString(const String& fileNamespace, const SAppDrawableValue& value, String& result);
		sl_bool _getDrawableValue(const String& fileNamespace, const SAppDrawableValue& value, Ref<Drawable>& result);
		sl_bool _checkDrawableValue(const String& fileNamespace, const SAppDrawableValue& value);
		sl_bool _checkDrawableName(const String& fileNamespace, const String& name, const Ref<XmlElement>& element, String* outName = sl_null, Ref<SAppDrawableResource>* outResource = sl_null);
		sl_bool _registerFileResources(const String& resourcePath, const String& fileDirPath, const Locale& locale);
		sl_bool _generateDrawablesCpp_File(SAppDrawableResource* res, StringBuffer& sbHeader, StringBuffer& sbCpp, StringBuffer& sbMap);
		Ref<Drawable> _getDrawableValue_File(SAppDrawableResource* res);
		sl_bool _parseNinePiecesDrawableResource(const String& fileNamespace, const Ref<XmlElement>& element);
		sl_bool _generateDrawablesCpp_NinePieces(SAppDrawableResource* res, StringBuffer& sbHeader, StringBuffer& sbCpp, StringBuffer& sbMap);
		Ref<Drawable> _getDrawableValue_NinePieces(SAppDrawableResource* res);
		sl_bool _parseNinePatchDrawableResource(const String& fileNamespace, const Ref<XmlElement>& element);
		sl_bool _generateDrawablesCpp_NinePatch(SAppDrawableResource* res, StringBuffer& sbHeader, StringBuffer& sbCpp, StringBuffer& sbMap);
		Ref<Drawable> _getDrawableValue_NinePatch(SAppDrawableResource* res);

		// Menu Resources
		sl_bool _parseMenuResource(const String& fileNamespace, const Ref<XmlElement>& element, sl_bool flagPopup);
		Ref<SAppMenuResourceItem> _parseMenuResourceItem(const String& fileNamespace, const Ref<XmlElement>& element, SAppMenuResource* menu, int platforms);
		sl_bool _generateMenusCpp(const String& targetPath);
		sl_bool _generateMenusCpp_Item(SAppMenuResource* resource, const String& parentName, int parentPlatforms, SAppMenuResourceItem* item, StringBuffer& sbHeader, StringBuffer& sbCpp, int tabLevel);
		sl_bool _getMenuAccessString(const String& fileNamespace, const SAppMenuValue& value, sl_bool flagForWindow, String& name, String& result);
		sl_bool _getMenuValue(const String& fileNamespace, const SAppMenuValue& value, Ref<Menu>& result);
		sl_bool _getMenuValue_Item(SAppMenuResource* resource, const Ref<Menu>& parent, SAppMenuResourceItem* item);
		sl_bool _checkMenuValue(const String& fileNamespace, const SAppMenuValue& value);
		sl_bool _checkMenuName(const String& fileNamespace, const String& name, const Ref<XmlElement>& element, String* outName = sl_null, Ref<SAppMenuResource>* outResource = sl_null);

		// Layout Resources
		sl_bool _parseLayoutStyle(const String& fileNamespace, const String& theme, const Ref<XmlElement>& element);
		sl_bool _parseLayoutInclude(const String& fileNamespace, const Ref<XmlElement>& element);
		sl_bool _parseLayoutUnit(const String& fileNamespace, const Ref<XmlElement>& element);
		Ref<SAppLayoutResource> _parseLayoutResource(const String& filePath, const String& fileNamespace, const Ref<XmlElement>& element, const String16& source, SAppLayoutResource* parent = sl_null, String* outChildLayoutName = sl_null, sl_bool* outFlagGeneratedName = sl_null);
		Ref<SAppLayoutResource> _openLayoutResource(SAppLayoutResource* parent, const String& name);
		sl_bool _checkLayoutResourceItemName(SAppLayoutResource* layout, const String& name, const Ref<XmlElement>& element, sl_bool flagRadioGroup = sl_false);
		sl_bool _parseLayoutResourceItem(SAppLayoutResource* layout, SAppLayoutResourceItem* item, SAppLayoutResourceItem* parent, const String16& source);
		void _registerLayoutCustomEvent(SAppLayoutResource* layout, const String& customEvent, const String& itemName, const String& itemEvent, sl_bool flagIterate);
		Ref<SAppLayoutResourceItem> _parseLayoutResourceItemChild(SAppLayoutResource* layout, SAppLayoutResourceItem* parentItem, const Ref<XmlElement>& element, const String16& source);
		sl_bool _generateLayoutsCpp(const String& targetPath);
		sl_bool _generateLayoutsCpp_Layout(const String& targetPath, SAppLayoutResource* layout);
		struct LayoutControlGenerateParams
		{
			StringBuffer* sbDeclare;
			StringBuffer* sbDefineInit;
			StringBuffer* sbDefineInitDelayed;
			StringBuffer* sbDefineLayout;
			StringBuffer* sbDefineSetData;
		};
		sl_bool _generateLayoutsCpp_Item(SAppLayoutResource* layout, SAppLayoutResourceItem* item, SAppLayoutResourceItem* parent, LayoutControlGenerateParams* params, const String& addStatement);
		sl_bool _simulateLayoutInWindow(SAppLayoutResource* layout, SAppSimulateLayoutParam& param);
		void _registerLayoutSimulationWindow(const Ref<SAppLayoutSimulationWindow>& window);
		void _removeLayoutSimulationWindow(const Ref<SAppLayoutSimulationWindow>& window);
		Ref<CRef> _simulateLayoutCreateOrLayoutItem(SAppLayoutSimulator* simulator, SAppLayoutResourceItem* item, SAppLayoutResourceItem* parent, CRef* parentItem, SAppLayoutOperation op);
		sl_ui_pos _getDimensionValue(const SAppDimensionValue& value);
		sl_real _getDimensionValue(const SAppDimensionFloatValue& value);
		sl_bool _getFontAccessString(const String& fileNamespace, const SAppFontValue& value, String& result);
		sl_bool _getFontValue(const String& fileNamespace, const SAppFontValue& value, Ref<Font>& result);
		sl_bool _getBorderAccessString(const String& fileNamespace, const SAppBorderValue& value, String& result);
		sl_bool _getBorderValue(const String& fileNamespace, const SAppBorderValue& value, PenDesc& result);
		Ref<SAppLayoutStyle> _lookupLayoutStyle(const String& fileNamespace, const String& theme, const String& styleName);
		sl_bool _parseStyleAttribute(const String& fileNamespace, const String& theme, SAppLayoutXmlItem* item);
		sl_bool _getXmlChildElements(List< Ref<XmlElement> >& ret, const String& fileNamespace, const String& theme, SAppLayoutXmlItem* item, const String& tagName);
		sl_bool _addXmlChildElements(List< Ref<XmlElement> >& ret, const String& fileNamespace, const String& theme, SAppLayoutStyle* style, const String& tagName);
		sl_bool _addXmlChildElements(List< Ref<XmlElement> >& ret, const String& fileNamespace, const String& theme, const Ref<XmlElement>& parent, const RefT<SAppLayoutXmlItem>& caller, const String& tagName);

		struct LayoutControlProcessParams : LayoutControlGenerateParams
		{
			SAppLayoutOperation op;
			String16 source;
			SAppLayoutResource* resource;
			SAppLayoutResourceItem* resourceItem;
			SAppLayoutResourceItem* parentResourceItem;
			String name;
			String addStatement;
			SAppLayoutSimulator* simulator;
			SAppLayoutSimulationWindow* window;
			Ref<CRef> viewItem;
			CRef* parentItem;
		};
		sl_bool _processLayoutResourceControl(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_View(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Window(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Page(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_ViewGroup(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Import(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Button(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Label(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Line(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Check(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Radio(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Edit(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Password(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_TextArea(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Image(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Select(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_SelectSwitch(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_ComboBox(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Scroll(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Linear(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_LinearIterate(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_TileLayout(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_TileIterate(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_List(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Collection(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Table(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_ListControl(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Render(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Tab(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Tree(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_TreeItem(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Split(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Web(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Progress(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Slider(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Switch(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Picker(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_DatePicker(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Pager(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Navigation(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Audio(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Video(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Camera(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Drawer(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Chat(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Refresh(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_ListBox(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_LabelList(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Pdf(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Map(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_GroupBox(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Grid(LayoutControlProcessParams* params);
		sl_bool _processLayoutResourceControl_Grid_ParseCellCreator(SAppLayoutGridCellAttributes& attr, const String& tagName, SAppLayoutXmlItem& xml);
		String _processLayoutResourceControl_Grid_GenerateCellCreator(SAppLayoutGridCellAttributes& attr);
		GridView::CellCreator _processLayoutResourceControl_Grid_SimulateCellCreator(SAppLayoutGridCellAttributes& attr);

	private:
		sl_bool m_flagOpened;
		String m_pathConf;

		SAppConfiguration m_conf;

		CMap< String, Ref<SAppRawResource> > m_raws;
		CMap< String, Ref<SAppDrawableResource> > m_drawables;
		CMap< String, Ref<SAppStringResource> > m_strings;
		CMap< String, Ref<SAppColorResource> > m_colors;
		CMap< String, Ref<SAppMenuResource> > m_menus;
		CHashMap< String, Ref<SAppLayoutStyle> > m_layoutStyles;
		CHashMap< String, Ref<SAppLayoutInclude> > m_layoutIncludes;
		CHashMap< String, SAppDimensionValue > m_layoutUnits;
		CMap< String, Ref<SAppLayoutResource> > m_layouts;

		CList< Ref<SAppLayoutSimulationWindow> > m_layoutSimulationWindows;
		SAppLayoutSimulationParams m_layoutSimulationParams;

		String m_currentFileNamespace;

		template <class MAP, class ITEM>
		static sl_bool getItemFromMap(const MAP& map, const String& fileNamespace, const String& name, String* outName, ITEM* _out);

		friend class SAppLayoutSimulator;
		friend class SAppLayoutSimulationWindow;
		friend class SAppDimensionBaseValue;
	};


}

#endif
