/*
 *   Copyright (c) 2008-2022 SLIBIO <https://github.com/SLIBIO>
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

#include "linkedin.h"

namespace slib
{

	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(LinkedIn, LoginParam)

	LinkedIn::LoginParam::LoginParam()
	{
		authorization.scopes.add_NoLock("r_liteprofile");
		authorization.scopes.add_NoLock("r_emailaddress");
	}

	void LinkedIn::LoginParam::addScopeForSharing()
	{
		authorization.scopes.addIfNotExist_NoLock("w_member_social");
	}


	SLIB_DEFINE_NESTED_CLASS_DEFAULT_MEMBERS(LinkedIn, ResolveUserUrlParam)

	LinkedIn::ResolveUserUrlParam::ResolveUserUrlParam()
	{
	}

	void LinkedIn::resolveUserUrl(const LinkedIn::ResolveUserUrlParam& param)
	{
		auto onComplete = param.onComplete;
		auto dialog = param.dialog;
		if (dialog.isNull()) {
			dialog = OAuthWebRedirectDialog::getDefault();
		}
		OAuthWebRedirectDialogParam dialogParam;
		dialogParam.url = "https://www.linkedin.com/in/";
		dialogParam.options = param.dialogOptions;
		auto weakDialog = dialog.toWeak();
		dialogParam.onRedirect = [weakDialog, onComplete](const String& _url) {
			if (_url.isEmpty()) {
				onComplete(sl_null);
				return;
			}
			if (_url.startsWith("https://www.linkedin.com/in/")) {
				Url url(_url);
				String name = url.path.substring(4);
				sl_size n = name.getLength();
				if (n > 1) {
					sl_char8* sz = name.getData();
					sl_bool flagValid = sl_true;
					if (flagValid) {
						for (sl_size i = 0; i < n; i++) {
							sl_char8 ch = sz[i];
							if (!(SLIB_CHAR_IS_ALNUM(ch) || ch == '-')) {
								flagValid = sl_false;
								break;
							}
						}
					}
					if (flagValid) {
						auto dialog = weakDialog.lock();
						if (dialog.isNotNull()) {
							dialog->close();
						}
						onComplete("https://www.linkedin.com/in/" + name);
					}
				}
			}
		};
		dialog->show(dialogParam);
	}

	void LinkedIn::resolveUserUrl(const Function<void(const String& url)>& onComplete)
	{
		ResolveUserUrlParam param;
		param.onComplete = onComplete;
		resolveUserUrl(param);
	}

}
