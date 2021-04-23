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

#if defined(SLIB_UI_IS_GTK)

#include "slib/ui/common_dialogs.h"

#include "slib/core/file.h"
#include "slib/network/url.h"
#include "slib/ui/core.h"
#include "slib/ui/platform.h"

namespace slib
{

	DialogResult AlertDialog::run()
	{
		return _runOnUiThread();
	}

	DialogResult AlertDialog::_run()
	{
		GtkWindow* hParent = UIPlatform::getWindowHandle(parent.get());
		
		GtkMessageType messgeType;
		switch (icon) {
			case AlertDialogIcon::Error:
				messgeType = GTK_MESSAGE_ERROR;
				break;
			case AlertDialogIcon::Warning:
				messgeType = GTK_MESSAGE_WARNING;
				break;
			case AlertDialogIcon::Question:
				messgeType = GTK_MESSAGE_QUESTION;
				break;
			case AlertDialogIcon::Information:
				messgeType = GTK_MESSAGE_INFO;
				break;
			default:
				messgeType = GTK_MESSAGE_OTHER;
				break;
		}
		
		StringCstr _text(text);
		GtkDialog* dialog = (GtkDialog*)(
			gtk_message_dialog_new(hParent,
								   GTK_DIALOG_DESTROY_WITH_PARENT,
								   messgeType,
								   GTK_BUTTONS_NONE,
								   "%s", _text.getData())
		);
		if (!dialog) {
			return DialogResult::Error;
		}
		
		StringCstr _caption(caption);
		gtk_window_set_title((GtkWindow*)dialog, _caption.getData());
		
		StringCstr szTitleOk = titleOk;
		if (titleOk.isEmpty()) {
			szTitleOk = GTK_STOCK_OK;
		}
		StringCstr szTitleCancel = titleCancel.getData();
		if (titleCancel.isEmpty()) {
			szTitleCancel = GTK_STOCK_CANCEL;
		}
		StringCstr szTitleYes = titleYes.getData();
		if (titleYes.isEmpty()) {
			szTitleYes = GTK_STOCK_YES;
		}
		StringCstr szTitleNo = titleNo.getData();
		if (titleNo.isEmpty()) {
			szTitleNo = GTK_STOCK_NO;
		}
		
		if (buttons == AlertDialogButtons::OkCancel) {
			gtk_dialog_add_button(dialog, szTitleOk.getData(), GTK_RESPONSE_OK);
			gtk_dialog_add_button(dialog, szTitleCancel.getData(), GTK_RESPONSE_CANCEL);
			gtk_dialog_set_alternative_button_order(dialog, GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);
		} else if (buttons == AlertDialogButtons::YesNo) {
			gtk_dialog_add_button(dialog, szTitleYes.getData(), GTK_RESPONSE_YES);
			gtk_dialog_add_button(dialog, szTitleNo.getData(), GTK_RESPONSE_NO);
			gtk_dialog_set_alternative_button_order(dialog, GTK_RESPONSE_YES, GTK_RESPONSE_NO, -1);
		} else if (buttons == AlertDialogButtons::YesNoCancel) {
			gtk_dialog_add_button(dialog, szTitleYes.getData(), GTK_RESPONSE_YES);
			gtk_dialog_add_button(dialog, szTitleNo.getData(), GTK_RESPONSE_NO);
			gtk_dialog_add_button(dialog, szTitleCancel.getData(), GTK_RESPONSE_CANCEL);
			gtk_dialog_set_alternative_button_order(dialog, GTK_RESPONSE_YES, GTK_RESPONSE_NO, GTK_RESPONSE_CANCEL, -1);
		} else {
			gtk_dialog_add_button(dialog, szTitleOk.getData(), GTK_RESPONSE_OK);
		}
		
		gint response = gtk_dialog_run(dialog);
		
		gtk_widget_destroy((GtkWidget*)dialog);
		
		switch (response) {
			case GTK_RESPONSE_OK:
				return DialogResult::Ok;
			case GTK_RESPONSE_YES:
				return DialogResult::Yes;
			case GTK_RESPONSE_NO:
				return DialogResult::No;
			default:
				break;
		}
		return DialogResult::Error;
	}

	void AlertDialog::show()
	{
		_showByRun();
	}

	sl_bool AlertDialog::_show()
	{
		return sl_false;
	}


	DialogResult FileDialog::run()
	{
		return _runOnUiThread();
	}

	DialogResult FileDialog::_run()
	{
		StringCstr szTitle = title;
		
		GtkWindow* hParent = UIPlatform::getWindowHandle(parent.get());
		
		GtkFileChooserAction action;
		const char* szButtonAccept;
		if (type == FileDialogType::SelectDirectory) {
			action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
			szButtonAccept = "Select";
		} else if (type == FileDialogType::OpenFile || type == FileDialogType::OpenFiles) {
			action = GTK_FILE_CHOOSER_ACTION_OPEN;
			szButtonAccept = GTK_STOCK_OPEN;
		} else if (type == FileDialogType::SaveFile) {
			action = GTK_FILE_CHOOSER_ACTION_SAVE;
			szButtonAccept = GTK_STOCK_SAVE;
		} else {
			return DialogResult::Error;
		}
		
		GtkFileChooserDialog* dialog = (GtkFileChooserDialog*)(
			gtk_file_chooser_dialog_new(szTitle.getData(),
										hParent,
										action,
										GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
										szButtonAccept, GTK_RESPONSE_ACCEPT,
										sl_null)
		);
		if (!dialog) {
			return DialogResult::Error;
		}
		
		GtkFileChooser* chooser = GTK_FILE_CHOOSER(dialog);
		
		gtk_window_set_title((GtkWindow*)dialog, szTitle.getData());
		
		if (type == FileDialogType::OpenFiles) {
			gtk_file_chooser_set_select_multiple(chooser, sl_true);
		} else {
			gtk_file_chooser_set_select_multiple(chooser, sl_false);
		}
		
		gtk_file_chooser_set_create_folders(chooser, sl_true);
		
		gtk_file_chooser_set_show_hidden(chooser, flagShowHiddenFiles?sl_true:sl_false);
		
		if (selectedPath.isNotEmpty()) {
			if (type != FileDialogType::SaveFile || File::isDirectory(selectedPath)) {
				StringCstr uri = Url::toFileUri(selectedPath);
				gtk_file_chooser_set_uri(chooser, uri.getData());
			} else {
				StringCstr selectedFile;
				sl_reg indexSlash = selectedPath.indexOf('/');
				if (indexSlash >= 0) {
					StringCstr selectedDir = Url::toFileUri(selectedPath.substring(0, indexSlash));
					selectedFile = selectedPath.substring(indexSlash + 1);
					gtk_file_chooser_set_current_folder_uri(chooser, selectedDir.getData());
				} else {
					selectedFile = selectedPath;
				}
				gtk_file_chooser_set_current_name(chooser, selectedFile.getData());
			}
		}
		
		if (filters.isNotEmpty()) {
			ListElements<FileDialogFilter> list(filters);
			for (sl_size i = 0; i < list.count; i++) {
				FileDialogFilter& filterDesc = list[i];
				GtkFileFilter* filter = gtk_file_filter_new();
				if (filter) {
					StringCstr filterTitle(filterDesc.title);
					gtk_file_filter_set_name(filter, filterTitle.getData());
					ListElements<String> patterns(filterDesc.patterns.split(";"));
					for (sl_size k = 0; k < patterns.count; k++) {
						StringCstr pattern(patterns[k]);
						gtk_file_filter_add_pattern(filter, pattern.getData());
					}
					gtk_file_chooser_add_filter(chooser, filter);
				}
			}
		}
		
		gint response = gtk_dialog_run((GtkDialog*)dialog);
		
		selectedPaths.removeAll();
		
		DialogResult ret = DialogResult::Error;
		if (response == GTK_RESPONSE_ACCEPT) {
			gchar* path = gtk_file_chooser_get_uri(chooser);
			if (path) {
				selectedPath = Url::getPathFromFileUri(path);
				g_free(path);
				if (type == FileDialogType::SaveFile) {
					if (defaultFileExt.isNotEmpty()) {
						String ext = File::getFileExtension(selectedPath);
						if (ext.isEmpty()) {
							selectedPath += ".";
							selectedPath += defaultFileExt;
						}
					}
				}
				GSList* list = gtk_file_chooser_get_uris(chooser);
				if (list) {
					GSList* item = list;
					do {
						selectedPaths.add(Url::getPathFromFileUri((char*)(item->data)));
						g_free(item->data);
						item = item->next;
					} while (item);
					g_slist_free(list);
				}
				ret = DialogResult::Ok;
			}
		} else {
			ret = DialogResult::Cancel;
		}
		
		gtk_widget_destroy((GtkWidget*)dialog);
		
		return ret;
	}

	void FileDialog::show()
	{
		_showByRun();
	}
	
	sl_bool FileDialog::_show()
	{
		return sl_false;
	}

}

#endif
