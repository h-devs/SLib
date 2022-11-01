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

#ifndef CHECKHEADER_SLIB_UI_COMMON_DIALOGS
#define CHECKHEADER_SLIB_UI_COMMON_DIALOGS

#include "view.h"

#include "../core/string.h"

namespace slib
{

	class SLIB_EXPORT AlertDialog : public Referable
	{
	public:
		AlertDialog();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(AlertDialog)

	public:
		DialogResult run();

		void show();

	public:
		Ref<Window> parent;
		String caption;
		String text;
		sl_bool flagHyperText;
		AlertButtons buttons;
		AlertIcon icon;

		String titleOK;
		String titleCancel;
		String titleYes;
		String titleNo;

		Function<void()> onOK;
		Function<void()> onCancel;
		Function<void()> onYes;
		Function<void()> onNo;
		Function<void()> onError;
		Function<void(DialogResult)> onComplete;

	public:
		DialogResult _run();

		sl_bool _show();

		sl_bool _showMobilePopup();

		void _onResult(DialogResult result);

	protected:
		DialogResult _runOnUiThread();

		DialogResult _runByShow();

		void _showOnUiThread();

		void _showByRun();

		AlertDialog* _getReferable();

	};

	enum class FileDialogType
	{
		SaveFile = 1,
		OpenFile = 2,
		OpenFiles = 3,
		SelectDirectory = 4
	};

	class FileDialogFilter
	{
	public:
		String title;

		// To specify multiple filter patterns for a single display string, use a semicolon to separate the patterns (for example, "*.TXT;*.DOC;*.BAK").
		String patterns;

	public:
		FileDialogFilter();

		FileDialogFilter(const String& title, const String& patterns);

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FileDialogFilter)

	};

	class SLIB_EXPORT FileDialog : public Referable
	{
	public:
		static List<String> openFiles(const Ref<Window>& parent);

		static String openFile(const Ref<Window>& parent);

		static String saveFile(const Ref<Window>& parent);

		static String selectDirectory(const Ref<Window>& parent);

	public:
		FileDialog();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(FileDialog)

	public:
		DialogResult run();

		void show();

		// To specify multiple filter patterns for a single display string, use a semicolon to separate the patterns (for example, "*.TXT;*.DOC;*.BAK").
		void addFilter(const String& title, const String& patterns);

	public:
		FileDialogType type;
		Ref<Window> parent;
		String title;
		sl_bool flagShowHiddenFiles;

		String defaultFileExt;

		List<FileDialogFilter> filters;

		DialogResult result;
		String selectedPath;
		List<String> selectedPaths;

		Function<void(FileDialog&)> onComplete;

	public:
		DialogResult _run();

		sl_bool _show();

		void _onResult(DialogResult result);

	protected:
		DialogResult _runOnUiThread();

		DialogResult _runByShow();

		void _showOnUiThread();

		void _showByRun();

		FileDialog* _getReferable();

	};

	class SLIB_EXPORT PromptDialog
	{
	public:
		PromptDialog();

		SLIB_DECLARE_CLASS_DEFAULT_MEMBERS(PromptDialog)

	public:
		String run();

		void show();

	public:
		Ref<Window> parent;
		String caption;
		String message;
		String defaultValue;

		Function<void(String&)> onOK;
		Function<void()> onCancel;

	};

}

#endif
