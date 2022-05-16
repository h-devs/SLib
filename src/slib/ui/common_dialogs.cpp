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

#include "slib/ui/common_dialogs.h"

#include "slib/core/event.h"

#include "slib/ui/core.h"
#include "slib/ui/label_view.h"
#include "slib/ui/button.h"

#include "../resources.h"

namespace slib
{
	
	namespace priv
	{
		namespace alert_dialog
		{
			
			class RunOnUiThread
			{
			public:
				AlertDialog* alert;
				Ref<Event> event;
				DialogResult result = DialogResult::Cancel;
				
			public:
				void run()
				{
					result = alert->_run();
					event->set();
				}
				
			};
			
			class RunByShowOnUiThread
			{
			public:
				DialogResult result = DialogResult::Error;
				
			public:
				void onComplete(DialogResult _result)
				{
					result = _result;
					UI::quitLoop();
				}
				
			};
			
			class RunByShowOnWorkingThread
			{
			public:
				DialogResult result = DialogResult::Error;
				Ref<Event> event;
				
			public:
				void onComplete(DialogResult _result)
				{
					result = _result;
					event->set();
				}
				
			};
			
			void ShowOnWorkingThread(AlertDialog* alert, RunByShowOnWorkingThread* m)
			{
				if (!(alert->_show())) {
					m->onComplete(DialogResult::Error);
				}
			}
			
			void ShowOnUiThread(const Ref<AlertDialog>& alert)
			{
				if (!(alert->_show())) {
					alert->_onResult(DialogResult::Error);
				}
			}

			void ShowOnUiThreadByRun(const Ref<AlertDialog>& alert)
			{
				DialogResult result = alert->_run();
				alert->_onResult(result);
			}

		}
	}
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(AlertDialog)
	
	AlertDialog::AlertDialog()
	{
		flagHyperText = sl_false;
		buttons = AlertButtons::Ok;
		icon = AlertIcon::None;
	}
	
	DialogResult AlertDialog::_runOnUiThread()
	{
		if (UI::isUiThread()) {
			return _run();
		}
		Ref<Event> ev = Event::create(sl_false);
		if (ev.isNotNull()) {
			priv::alert_dialog::RunOnUiThread m;
			m.alert = this;
			m.event = Move(ev);
			UI::dispatchToUiThread(SLIB_FUNCTION_MEMBER(&m, run));
			m.event->wait();
			return m.result;
		}
		return DialogResult::Error;
	}
	
	DialogResult AlertDialog::_runByShow()
	{
		Ref<AlertDialog> alert = new AlertDialog(*this);
		if (alert.isNull()) {
			return DialogResult::Error;
		}
		if (UI::isUiThread()) {
			priv::alert_dialog::RunByShowOnUiThread m;
			alert->onComplete = SLIB_FUNCTION_MEMBER(&m, onComplete);
#if defined(SLIB_UI_IS_IOS)
			if (alert->_showMobilePopup()) {
				UI::runLoop();
				return m.result;
			}
#else
			if (alert->_show()) {
				UI::runLoop();
				return m.result;
			}
#endif
		} else {
			Ref<Event> ev = Event::create(sl_false);
			if (ev.isNotNull()) {
				priv::alert_dialog::RunByShowOnWorkingThread m;
				m.event = Move(ev);
				alert->onComplete = SLIB_FUNCTION_MEMBER(&m, onComplete);
				UI::dispatchToUiThread(Function<void()>::bind(&(priv::alert_dialog::ShowOnWorkingThread), alert.get(), &m));
				m.event->wait();
				return m.result;
			}
		}
		return DialogResult::Error;
	}
	
	void AlertDialog::_showOnUiThread()
	{
		Ref<AlertDialog> alert = _getReferable();
		if (alert.isNotNull()) {
			if (UI::isUiThread()) {
				priv::alert_dialog::ShowOnUiThread(alert);
			} else {
				UI::dispatchToUiThread(Function<void()>::bind(&(priv::alert_dialog::ShowOnUiThread), alert));
			}
		}
	}

	void AlertDialog::_showByRun()
	{
		Ref<AlertDialog> alert = _getReferable();
		if (alert.isNotNull()) {
			UI::dispatchToUiThread(Function<void()>::bind(&(priv::alert_dialog::ShowOnUiThreadByRun), alert));
		}
	}
	
	void AlertDialog::_onResult(DialogResult result)
	{
		onComplete(result);
		switch (result) {
			case DialogResult::Ok:
				onOk();
				break;
			case DialogResult::Yes:
				onYes();
				break;
			case DialogResult::No:
				onNo();
				break;
			case DialogResult::Cancel:
				onCancel();
				break;
			case DialogResult::Error:
				onError();
				if (onComplete.isNull() && onError.isNull()) {
					if (buttons == AlertButtons::Ok) {
						onOk();
					} else if (buttons == AlertButtons::YesNo) {
						onNo();
					} else {
						onCancel();
					}
				}
			default:
				break;
		}
	}
	
	sl_bool AlertDialog::_showMobilePopup()
	{
		Ref<MobileApp> app = MobileApp::getApp();
		if (app.isNull()) {
			return sl_false;
		}

		Ref<ui::MobileAlertDialog> dlg = new ui::MobileAlertDialog;
		
		if (caption.isNotNull()) {
			dlg->txtTitle->setText(caption, UIUpdateMode::Init);
		} else {
			dlg->txtTitle->setVisibility(Visibility::Gone, UIUpdateMode::Init);
		}
		if (flagHyperText) {
			dlg->txtContent->setHyperText(text, UIUpdateMode::Init);
		} else {
			dlg->txtContent->setText(text, UIUpdateMode::Init);
		}
		
		auto alert = ToRef(this);
		dlg->btnOK->setVisibility(Visibility::Gone, UIUpdateMode::Init);
		if (titleOk.isNotNull()) {
			dlg->btnOK->setText(titleOk, UIUpdateMode::Init);
		}
		dlg->btnOK->setOnClick([alert](View* view) {
			alert->_onResult(DialogResult::Ok);
			view->getNearestViewPage()->close();
		});
		
		dlg->btnYes->setVisibility(Visibility::Gone, UIUpdateMode::Init);
		if (titleYes.isNotNull()) {
			dlg->btnYes->setText(titleYes, UIUpdateMode::Init);
		}
		dlg->btnYes->setOnClick([alert](View* view) {
			alert->_onResult(DialogResult::Yes);
			view->getNearestViewPage()->close();
		});

		dlg->btnNo->setVisibility(Visibility::Gone, UIUpdateMode::Init);
		if (titleNo.isNotNull()) {
			dlg->btnNo->setText(titleNo, UIUpdateMode::Init);
		}
		dlg->btnNo->setOnClick([alert](View* view) {
			alert->_onResult(DialogResult::No);
			view->getNearestViewPage()->close();
		});

		dlg->btnCancel->setVisibility(Visibility::Gone, UIUpdateMode::Init);
		if (titleCancel.isNotNull()) {
			dlg->btnCancel->setText(titleCancel, UIUpdateMode::Init);
		}
		dlg->btnCancel->setOnClick([alert](View* view) {
			alert->_onResult(DialogResult::Cancel);
			view->getNearestViewPage()->close();
		});

		dlg->setOnBack([alert](ViewPage*, UIEvent* ev) {
			if (alert->buttons == AlertButtons::YesNo) {
				ev->preventDefault();
			} else {
				alert->_onResult(DialogResult::Cancel);
			}
		});
		
		if (buttons == AlertButtons::OkCancel) {
			dlg->btnOK->setVisibility(Visibility::Visible, UIUpdateMode::Init);
			dlg->btnCancel->setVisibility(Visibility::Visible, UIUpdateMode::Init);
			dlg->btnCancel->removeAllChildren(UIUpdateMode::Init);
		} else if (buttons == AlertButtons::YesNo) {
			dlg->btnYes->setVisibility(Visibility::Visible, UIUpdateMode::Init);
			dlg->btnNo->setVisibility(Visibility::Visible, UIUpdateMode::Init);
			dlg->btnNo->removeAllChildren(UIUpdateMode::Init);
		} else if (buttons == AlertButtons::YesNoCancel) {
			dlg->btnYes->setVisibility(Visibility::Visible, UIUpdateMode::Init);
			dlg->btnNo->setVisibility(Visibility::Visible, UIUpdateMode::Init);
			dlg->btnCancel->setVisibility(Visibility::Visible, UIUpdateMode::Init);
			dlg->btnCancel->removeAllChildren(UIUpdateMode::Init);
		} else {
			dlg->btnOK->setVisibility(Visibility::Visible, UIUpdateMode::Init);
			dlg->btnOK->removeAllChildren(UIUpdateMode::Init);
			dlg->setCloseOnClickBackground();
		}

		app->popupPage(dlg);
		
		return sl_true;
		
	}
	
	AlertDialog* AlertDialog::_getReferable()
	{
		if (getReferenceCount() > 0) {
			return this;
		} else {
			return new AlertDialog(*this);
		}
	}

	
	namespace priv
	{
		namespace file_dialog
		{
			
			class RunOnUiThread
			{
			public:
				FileDialog* dlg;
				Ref<Event> event;
				DialogResult result = sl_false;
				
			public:
				void run()
				{
					result = dlg->_run();
					event->set();
				}
				
			};
			
			class RunByShowOnUiThread
			{
			public:
				DialogResult result = DialogResult::Error;
				String path;
				List<String> list;

			public:
				void onComplete(FileDialog& dialog)
				{
					result = dialog.result;
					path = dialog.selectedPath;
					list = dialog.selectedPaths;
					UI::quitLoop();
				}
				
			};
			
			class RunByShowOnWorkingThread
			{
			public:
				Ref<Event> event;
				DialogResult result = DialogResult::Error;
				String path;
				List<String> list;

			public:
				void onComplete(FileDialog& dialog)
				{
					result = dialog.result;
					path = dialog.selectedPath;
					list = dialog.selectedPaths;
					event->set();
				}
				
			};
			
			void ShowOnWorkingThread(FileDialog* dialog, RunByShowOnWorkingThread* m)
			{
				if (!(dialog->_show())) {
					dialog->result = DialogResult::Error;
					m->onComplete(*dialog);
				}
			}
			
			void ShowOnUiThread(const Ref<FileDialog>& dialog)
			{
				if (!(dialog->_show())) {
					dialog->_onResult(DialogResult::Error);
				}
			}
			
			void ShowOnUiThreadByRun(const Ref<FileDialog>& dialog)
			{
				DialogResult result = dialog->_run();
				dialog->_onResult(result);
			}
			
		}
	}
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FileDialogFilter)

	FileDialogFilter::FileDialogFilter()
	{
	}
	
	FileDialogFilter::FileDialogFilter(const String& _title, const String& _patterns): title(_title), patterns(_patterns)
	{
	}
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(FileDialog)
	
	FileDialog::FileDialog()
	{
		type = FileDialogType::OpenFile;
		flagShowHiddenFiles = sl_true;
	}

	void FileDialog::addFilter(const String& title, const String& patterns)
	{
		filters.add(FileDialogFilter(title, patterns));
	}

	List<String> FileDialog::openFiles(const Ref<Window>& parent)
	{
		FileDialog dlg;
		dlg.type = FileDialogType::OpenFiles;
		dlg.parent = parent;
		if (dlg.run() == DialogResult::Ok) {
			return dlg.selectedPaths;
		}
		return sl_null;
	}
	
	String FileDialog::openFile(const Ref<Window>& parent)
	{
		FileDialog dlg;
		dlg.type = FileDialogType::OpenFile;
		dlg.parent = parent;
		if (dlg.run() == DialogResult::Ok) {
			return dlg.selectedPath;
		}
		return sl_null;
	}
	
	String FileDialog::saveFile(const Ref<Window>& parent)
	{
		FileDialog dlg;
		dlg.type = FileDialogType::SaveFile;
		dlg.parent = parent;
		if (dlg.run() == DialogResult::Ok) {
			return dlg.selectedPath;
		}
		return sl_null;
	}
	
	String FileDialog::selectDirectory(const Ref<Window>& parent)
	{
		FileDialog dlg;
		dlg.type = FileDialogType::SelectDirectory;
		dlg.parent = parent;
		if (dlg.run() == DialogResult::Ok) {
			return dlg.selectedPath;
		}
		return sl_null;
	}
	
	DialogResult FileDialog::_runOnUiThread()
	{
		if (UI::isUiThread()) {
			return _run();
		}
		Ref<Event> ev = Event::create(sl_false);
		if (ev.isNotNull()) {
			priv::file_dialog::RunOnUiThread m;
			m.dlg = this;
			m.event = Move(ev);
			UI::dispatchToUiThread(SLIB_FUNCTION_MEMBER(&m, run));
			m.event->wait();
			return m.result;
		}
		return sl_false;
	}
	
	DialogResult FileDialog::_runByShow()
	{
		Ref<FileDialog> dialog = new FileDialog(*this);
		if (dialog.isNull()) {
			return DialogResult::Error;
		}
		if (UI::isUiThread()) {
			priv::file_dialog::RunByShowOnUiThread m;
			dialog->onComplete = SLIB_FUNCTION_MEMBER(&m, onComplete);
			if (dialog->_show()) {
				UI::runLoop();
				result = m.result;
				selectedPath = m.path;
				selectedPaths = m.list;
				return m.result;
			}
		} else {
			Ref<Event> ev = Event::create(sl_false);
			if (ev.isNotNull()) {
				priv::file_dialog::RunByShowOnWorkingThread m;
				m.event = Move(ev);
				dialog->onComplete = SLIB_FUNCTION_MEMBER(&m, onComplete);
				UI::dispatchToUiThread(Function<void()>::bind(&(priv::file_dialog::ShowOnWorkingThread), dialog.get(), &m));
				m.event->wait();
				result = m.result;
				selectedPath = m.path;
				selectedPaths = m.list;
				return m.result;
			}
		}
		return DialogResult::Error;
	}
	
	void FileDialog::_showOnUiThread()
	{
		Ref<FileDialog> dialog = _getReferable();
		if (dialog.isNotNull()) {
			if (UI::isUiThread()) {
				priv::file_dialog::ShowOnUiThread(dialog);
			} else {
				UI::dispatchToUiThread(Function<void()>::bind(&(priv::file_dialog::ShowOnUiThread), dialog));
			}
		}
	}
	
	void FileDialog::_showByRun()
	{
		Ref<FileDialog> dialog = _getReferable();
		if (dialog.isNotNull()) {
			UI::dispatchToUiThread(Function<void()>::bind(&(priv::file_dialog::ShowOnUiThreadByRun), dialog));
		}
	}
	
	void FileDialog::_onResult(DialogResult _result)
	{
		result = _result;
		onComplete(*this);
	}
	
	FileDialog* FileDialog::_getReferable()
	{
		if (getReferenceCount() > 0) {
			return this;
		} else {
			return new FileDialog(*this);
		}
	}
	
#if !defined(SLIB_UI_IS_WIN32) && !defined(SLIB_UI_IS_MACOS) && !defined(SLIB_UI_IS_GTK)
	DialogResult FileDialog::run()
	{
		return DialogResult::Error;
	}
	
	DialogResult FileDialog::_run()
	{
		return DialogResult::Error;
	}
	
	void FileDialog::show()
	{
		_onResult(DialogResult::Error);
	}
	
	sl_bool FileDialog::_show()
	{
		return sl_false;
	}
#endif
	
}
