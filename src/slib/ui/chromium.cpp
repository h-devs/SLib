/*
 *   Copyright (c) 2008-2019 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/ui/chromium.h"

#if defined(SLIB_UI_IS_WIN32) || defined(SLIB_UI_IS_MACOS)
#	define SUPPORT_CHROMIUM
#endif

#ifdef SUPPORT_CHROMIUM

#include "view_win32.h"
#include "view_macos.h"

#include "slib/core/file.h"
#include "slib/core/thread.h"
#include "slib/core/safe_static.h"
#include "slib/network/http_common.h"

#ifdef SLIB_UI_IS_WIN32
#	pragma comment(lib, "libcef_dll_wrapper.lib")
#endif

#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/views/cef_browser_view.h"
#include "include/wrapper/cef_helpers.h"
#include "include/wrapper/cef_stream_resource_handler.h"
#include "include/wrapper/cef_closure_task.h"

#ifdef SLIB_UI_IS_MACOS

#	include "include/cef_application_mac.h"
#	include "include/wrapper/cef_library_loader.h"

@interface SLIBChromiumApplication : NSApplication<CefAppProtocol>
{
	@private
	BOOL handlingSendEvent_;
}
@end

#endif

#endif

namespace slib
{
	
	SLIB_DEFINE_OBJECT(ChromiumView, WebView)
	
	ChromiumView::ChromiumView()
	{
	}
	
	ChromiumView::~ChromiumView()
	{
	}
	
#ifdef SUPPORT_CHROMIUM
	
	namespace priv
	{
		
#ifdef SLIB_UI_IS_MACOS
		namespace ui_core
		{
			void SetCustomMessageLoop(const Function<void()>& func);
			void SetCustomQuitApp(const Function<void()>& func);
		}
#endif
		
		namespace chromium
		{
			
#ifdef SLIB_UI_IS_WIN32
			typedef Win32_ViewInstance BaseViewInstance;
#endif
#ifdef SLIB_UI_IS_MACOS
			typedef macOS_ViewInstance BaseViewInstance;
#endif
			
			class ChromiumApp;
			class ChromiumHandler;
			class ChromiumViewInstance;
			
			class StaticContext
			{
			public:
				CefRefPtr<ChromiumApp> app;
				CefRefPtr<ChromiumHandler> handler;
				List< CefRefPtr<CefBrowser> > browsers;
#ifdef SLIB_UI_IS_MACOS
				HashMap< void*, WeakRef<ChromiumViewInstance> > instances;
#endif
				
			public:
				StaticContext();
				
				~StaticContext();
				
			};
			
			SLIB_SAFE_STATIC_GETTER(StaticContext, GetStaticContext)
			
			class ChromiumSendMessageHandler : public CefV8Handler
			{
			public:
				ChromiumApp* app;
				CefRefPtr<CefBrowser> browser;
				CefRefPtr<CefFrame> frame;
				
			public:
				bool Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception) override;
				
			private:
				IMPLEMENT_REFCOUNTING(ChromiumSendMessageHandler);
			};
			
			class ChromiumApp : public CefApp, public CefBrowserProcessHandler, public CefRenderProcessHandler
			{
			public:
				StaticContext* m_context;
				
			public:
				ChromiumApp()
				{
				}
				
			public:
				CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override
				{
					return this;
				}
				
				CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override
				{
					return this;
				}
				
				// CefBrowserProcessHandler
				void OnContextInitialized() override
				{
				}
				
				// CefRenderProcessHandler
				void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override
				{
					CefRefPtr<CefV8Value> global = context->GetGlobal();
					CefRefPtr<ChromiumSendMessageHandler> handler = new ChromiumSendMessageHandler;
					handler->app = this;
					handler->browser = browser;
					handler->frame = frame;
					CefRefPtr<CefV8Value> slib = CefV8Value::CreateObject(sl_null, sl_null);
					CefRefPtr<CefV8Value> send = CefV8Value::CreateFunction("send", handler);
					slib->SetValue("send", send, V8_PROPERTY_ATTRIBUTE_NONE);
					global->SetValue("slib", slib, V8_PROPERTY_ATTRIBUTE_NONE);
				}
				
			private:
				IMPLEMENT_REFCOUNTING(ChromiumApp);
			};
			
			class SLIB_EXPORT EqualsCefBrowser
			{
			public:
				sl_bool operator()(const CefRefPtr<CefBrowser>& a, const CefRefPtr<CefBrowser>& b) const
				{
					return a->IsSame(b);
				}
				
			};
			
			static String GetString(const CefString& str)
			{
				if (str.length()) {
					return String::create((sl_char16*)(str.c_str()));
				}
				return sl_null;
			}
			
			static CefString GetCefString(const StringParam& _str)
			{
				StringData16 str(_str);
				return CefString((typename CefString::char_type*)(str.getData()), str.getLength(), true);
			}
			
			static String GetStringValue(CefRefPtr<CefV8Value> value)
			{
				if (value.get()) {
					if (value->IsString()) {
						return GetString(value->GetStringValue());
					} else if (value->IsBool()) {
						return String::fromBoolean(value->GetBoolValue());
					} else if (value->IsInt()) {
						return String::fromInt(value->GetIntValue());
					} else if (value->IsUInt()) {
						return String::fromUint32(value->GetUIntValue());
					} else if (value->IsDouble()) {
						return String::fromDouble(value->GetDoubleValue());
					}
				}
				return sl_null;
			}
			
			bool ChromiumSendMessageHandler::Execute(const CefString& name, CefRefPtr<CefV8Value> object, const CefV8ValueList& arguments, CefRefPtr<CefV8Value>& retval, CefString& exception)
			{
				if (name == "send" && arguments.size() == 2) {
					String str = "slib_send::" + GetStringValue(arguments.at(0)) + "::" + GetStringValue(arguments.at(1));
					CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(GetCefString(str));
					frame->SendProcessMessage(PID_BROWSER, message);
					return true;
				}
				return false;
			}
			
			class ChromiumViewHelper : public ChromiumView
			{
			public:
				void apply(ChromiumViewInstance* instance, CefWindowInfo& windowInfo);
				
				friend class ChromiumViewInstance;
			};
			
			class OfflineContentHandler : public CefResourceRequestHandler
			{
			public:
				String m_content;
				
			public:
				OfflineContentHandler(const String& content): m_content(content)
				{
				}
				
			public:
				CefRefPtr<CefResourceHandler> GetResourceHandler(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request) override
				{
					CefRefPtr<CefStreamReader> stream = CefStreamReader::CreateForData(m_content.getData(), m_content.getLength());
					return new CefStreamResourceHandler("text/html", stream);
				}
				
			private:
				IMPLEMENT_REFCOUNTING(OfflineContentHandler);
			};
			
			class ChromiumViewInstance : public BaseViewInstance, public IWebViewInstance
			{
				SLIB_DECLARE_OBJECT

			public:
				CefRefPtr<CefBrowserHost> m_host;
				CefRefPtr<CefBrowser> m_browser;

				sl_bool m_flagLoadOffline;
				AtomicString m_title;

				sl_bool m_flagResizeAfterCreate;
				sl_bool m_flagReloadAfterCreate;

			public:
				ChromiumViewInstance();

				~ChromiumViewInstance();

			public:
				Ref<ChromiumViewHelper> getHelper()
				{
					return CastRef<ChromiumViewHelper>(getView());
				}

				void initialize(View* _view) override
				{
					ChromiumViewHelper* view = (ChromiumViewHelper*)_view;
					CefWindowInfo windowInfo;
#if defined(SLIB_UI_IS_WIN32)
					RECT rc;
					rc.left = 0;
					rc.top = 0;
					rc.right = (LONG)(view->getWidth());
					rc.bottom = (LONG)(view->getHeight());
					windowInfo.SetAsChild(getHandle(), rc);
#elif defined(SLIB_UI_IS_MACOS)
					windowInfo.SetAsChild((__bridge void*)(getHandle()), 0, 0, (int)(view->getWidth()), (int)(view->getHeight()));
#endif
					view->apply(this, windowInfo);
				}

				void refreshSize(WebView* view) override
				{
					CefBrowserHost* host = m_host.get();
					if (host) {
#ifdef SLIB_UI_IS_WIN32
						HWND hWnd = host->GetWindowHandle();
						SetWindowPos(hWnd, NULL, 0, 0, (int)(view->getWidth()), (int)(view->getHeight()), SWP_NOMOVE | SWP_NOZORDER);
#endif
#ifdef SLIB_UI_IS_MACOS
						NSView* handle = (__bridge NSView*)(host->GetWindowHandle());
						[handle setFrame : NSMakeRect(0, 0, (CGFloat)(view->getWidth()), (CGFloat)(view->getHeight()))];
#endif
					} else {
						m_flagResizeAfterCreate = sl_true;
					}
				}

				void load(WebView* view) override
				{
					ChromiumViewHelper* helper = static_cast<ChromiumViewHelper*>(view);
					CefBrowser* browser = m_browser.get();
					if (browser) {
						m_flagLoadOffline = helper->m_flagOfflineContent;
						browser->GetMainFrame()->LoadURL(GetCefString(helper->m_urlOrigin));
					} else {
						m_flagReloadAfterCreate = sl_true;
					}
				}

				sl_bool getURL(WebView* view, String& _out) override
				{
					CefBrowser* browser = m_browser.get();
					if (browser) {
						_out = GetString(browser->GetMainFrame()->GetURL());
						return sl_true;
					}
					return sl_false;
				}

				sl_bool getPageTitle(WebView* view, String& _out) override
				{
					_out = m_title;
					return sl_true;
				}

				void goBack(WebView* view) override
				{
					CefBrowser* browser = m_browser.get();
					if (browser) {
						browser->GoBack();
					}
				}

				void goForward(WebView* view) override
				{
					CefBrowser* browser = m_browser.get();
					if (browser) {
						browser->GoForward();
					}
				}

				void reload(WebView* view) override
				{
					CefBrowser* browser = m_browser.get();
					if (browser) {
						browser->Reload();
					}
				}

				void runJavaScript(WebView* view, const StringParam& script) override
				{
					CefBrowser* browser = m_browser.get();
					if (browser) {
						CefRefPtr<CefFrame> frame = browser->GetMainFrame();
						frame->ExecuteJavaScript(GetCefString(script), frame->GetURL(), 0);
					}
				}

				void setZoomLevel(WebView* view, float level) override
				{
					CefBrowserHost* host = m_host.get();
					if (host) {
						host->SetZoomLevel(level);
					}
				}

				void find(WebView* view, const StringParam& text, const FindOptions& options) override
				{
					CefBrowserHost* host = m_host.get();
					if (host) {
						host->Find(0, GetCefString(text), !(options & FindOptions::Backward), options & FindOptions::MatchCase, false);
					}
				}

				void stopFinding(WebView* view) override
				{
					CefBrowserHost* host = m_host.get();
					if (host) {
						host->StopFinding(true);
					}
				}
				
#ifdef SLIB_UI_IS_WIN32
				LRESULT processWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam) override
				{
					HWND handle = getHandle();
					if (handle) {
						switch (msg) {
							case WM_PAINT:
								if (m_browser.get()) {
									PAINTSTRUCT ps;
									BeginPaint(m_handle, &ps);
									EndPaint(m_handle, &ps);
									return 0;
								}
								break;
							case WM_ERASEBKGND:
								return TRUE;
						}
					}
					return Win32_ViewInstance::processWindowMessage(msg, wParam, lParam);
				}
#endif
				
				void closeBrowsers()
				{
					CefBrowserHost* host = m_host.get();
					if (host) {
						host->CloseBrowser(sl_true);
						m_host = sl_null;
					}
					CefBrowser* browser = m_browser.get();
					if (browser) {
						m_browser = sl_null;
					}
				}
				
				void onCreatedBrowser(CefRefPtr<CefBrowser> browser)
				{
					m_browser = browser;
					m_host = browser->GetHost();
					Ref<ChromiumViewHelper> helper = getHelper();
					if (helper.isNotNull()) {
						if (m_flagResizeAfterCreate) {
							refreshSize(helper.get());
						}
						if (m_flagReloadAfterCreate) {
							load(helper.get());
						}
					}
				}
				
				CefRefPtr<CefResourceRequestHandler> onInterceptNavigationRequest(CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request)
				{
					Ref<ChromiumViewHelper> helper = getHelper();
					if (helper.isNotNull()) {
						helper->dispatchStartLoad(GetString(request->GetURL()));
						if (m_flagLoadOffline) {
							if (frame->IsMain()) {
								m_flagLoadOffline = sl_false;
								return new OfflineContentHandler(helper->m_offlineContentHTML);
							}
						}
					}
					return sl_null;
				}
				
				void onLoadEnd(const CefString& url)
				{
					Ref<ChromiumViewHelper> helper = getHelper();
					if (helper.isNotNull()) {
						helper->dispatchFinishLoad(GetString(url), sl_false);
					}
				}
				
				void onLoadError(const CefString& url, const CefString& errorText)
				{
					Ref<ChromiumViewHelper> helper = getHelper();
					if (helper.isNotNull()) {
						helper->dispatchFinishLoad(GetString(url), sl_true);
					}
				}
				
				void onReceivedMessage(const String& name, const String& param)
				{
					Ref<ChromiumViewHelper> helper = getHelper();
					if (helper.isNotNull()) {
						helper->dispatchMessageFromJavaScript(name, param);
					}
				}
				
			};
			
			SLIB_DEFINE_OBJECT(ChromiumViewInstance, BaseViewInstance)
			
			class ChromiumHandler : public CefClient, public CefDisplayHandler, public CefLifeSpanHandler, public CefLoadHandler, public CefRequestHandler, public CefKeyboardHandler
			{
			public:
				StaticContext * m_context;

			public:
				Ref<ChromiumViewInstance> getInstance(CefRefPtr<CefBrowser> browser)
				{
					if (browser.get()) {
						CefRefPtr<CefBrowserHost> host = browser->GetHost();
						if (host.get()) {
#if defined(SLIB_UI_IS_WIN32)
							HWND hWnd = host->GetWindowHandle();
							if (hWnd) {
								HWND hWndParent = GetParent(hWnd);
								return CastRef<ChromiumViewInstance>(UIPlatform::getViewInstance(hWndParent));
							}
#endif
#if defined(SLIB_UI_IS_MACOS)
							StaticContext* context = GetStaticContext();
							if (!context) {
								return sl_null;
							}
							void* handle = host->GetWindowHandle();
							Ref<ChromiumViewInstance> instance = context->instances.getValue(handle);
							if (instance.isNotNull()) {
								return instance;
							}
							if ([NSThread isMainThread]) {
								NSView* view = (__bridge NSView*)(handle);
								if (view != nil) {
									NSView* parent = [view superview];
									instance = CastRef<ChromiumViewInstance>(UIPlatform::getViewInstance(parent));
									if (instance.isNotNull()) {
										context->instances.put(handle, instance);
										return instance;
									}
								}
							}
#endif
						}
					}
					return sl_null;
				}

				CefRefPtr<CefDisplayHandler> GetDisplayHandler() override
				{
					return this;
				}
				
				CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override
				{
					return this;
				}
				
				CefRefPtr<CefLoadHandler> GetLoadHandler() override
				{
					return this;
				}
				
				CefRefPtr<CefRequestHandler> GetRequestHandler() override
				{
					return this;
				}

				CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override
				{
					return this;
				}
				
				// CefDisplayHandler
				void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) override
				{
					Ref<ChromiumViewInstance> instance = getInstance(browser);
					if (instance.isNotNull()) {
						instance->m_title = GetString(title);
					}
				}
				
				// CefLifeSpanHandler
				bool OnBeforePopup(
					CefRefPtr<CefBrowser> browser,
					CefRefPtr<CefFrame> frame,
					const CefString& target_url,
					const CefString& target_frame_name,
					CefLifeSpanHandler::WindowOpenDisposition target_disposition,
					bool user_gesture,
					const CefPopupFeatures& popupFeatures,
					CefWindowInfo& windowInfo,
					CefRefPtr<CefClient>& client,
					CefBrowserSettings& settings,
					CefRefPtr<CefDictionaryValue>& extra_info,
					bool* no_javascript_access
				) override
				{
					return false;
				}
				
				void OnAfterCreated(CefRefPtr<CefBrowser> browser) override
				{
					Ref<ChromiumViewInstance> instance = getInstance(browser);
					if (instance.isNotNull()) {
						instance->onCreatedBrowser(browser);
					}
					StaticContext* context = GetStaticContext();
					if (context) {
						context->browsers.addIfNotExist(browser);
					}
				}
				
				void OnBeforeClose(CefRefPtr<CefBrowser> browser) override
				{
					StaticContext* context = GetStaticContext();
					if (context) {
						context->browsers.remove(browser, EqualsCefBrowser());
#if defined(SLIB_UI_IS_MACOS)
						CefRefPtr<CefBrowserHost> host = browser->GetHost();
						if (host.get()) {
							context->instances.remove(host->GetWindowHandle());
						}
#endif
					}
				}
				
				// CefLoadHandler
				void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override
				{
					Ref<ChromiumViewInstance> instance = getInstance(browser);
					if (instance.isNotNull()) {
						instance->onLoadEnd(frame->GetURL());
					}
				}
				
				void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode, const CefString& errorText, const CefString& failedUrl) override
				{
					Ref<ChromiumViewInstance> instance = getInstance(browser);
					if (instance.isNotNull()) {
						instance->onLoadError(failedUrl, errorText);
					}
				}
				
				// CefRequestHandler
				CefRefPtr<CefResourceRequestHandler> GetResourceRequestHandler(
					CefRefPtr<CefBrowser> browser,
					CefRefPtr<CefFrame> frame,
					CefRefPtr<CefRequest> request,
					bool is_navigation,
					bool is_download,
					const CefString& request_initiator,
					bool& disable_default_handling
				) override
				{
					if (is_navigation) {
						Ref<ChromiumViewInstance> instance = getInstance(browser);
						if (instance.isNotNull()) {
							return instance->onInterceptNavigationRequest(frame, request);
						}
					}
					return NULL;
				}
				
				bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override
				{
					String name = GetString(message->GetName());
					if (name.startsWith("slib_send::")) {
						Ref<ChromiumViewInstance> instance = getInstance(browser);
						if (instance.isNotNull()) {
							name = name.substring(11);
							String param;
							sl_reg index = name.indexOf("::");
							if (index >= 0) {
								param = name.substring(index + 2);
								name = name.substring(0, index);
							}
							instance->onReceivedMessage(name, param);
							return true;
						}
					}
					return false;
				}

				bool OnPreKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent& ev, CefEventHandle os_event, bool* is_keyboard_shortcut) override
				{
					Ref<ChromiumViewInstance> instance = getInstance(browser);
					if (instance.isNull()) {
						return false;
					}
					Ref<ChromiumView> view = CastRef<ChromiumView>(instance->getView());
					if (view.isNull()) {
						return false;
					}
					do {
						Keycode keycode = UIEvent::getKeycodeFromWin32Keycode(ev.windows_key_code);
						UIAction action;
						if (ev.type == KEYEVENT_KEYDOWN || ev.type == KEYEVENT_RAWKEYDOWN) {
							action = UIAction::KeyDown;
						} else if (ev.type == KEYEVENT_KEYUP) {
							action = UIAction::KeyUp;
						} else {
							break;
						}
						Ref<UIEvent> _ev = UIEvent::createKeyEvent(action, keycode, 0, Time::now());
						if (_ev.isNotNull()) {
							if (ev.modifiers & EVENTFLAG_ALT_DOWN) {
								_ev->setAltKey();
							}
							if (ev.modifiers & EVENTFLAG_CONTROL_DOWN) {
								_ev->setControlKey();
							}
							if (ev.modifiers & EVENTFLAG_SHIFT_DOWN) {
								_ev->setShiftKey();
							}
							if (ev.modifiers & EVENTFLAG_COMMAND_DOWN) {
								_ev->setCommandKey();
							}
							_ev->addFlag(UIEventFlags::DispatchToParent | UIEventFlags::NotDispatchToChildren);
							instance->onKeyEvent(_ev.get());
							if (_ev->isPreventedDefault()) {
								return true;
							}
						}
					} while (0);
					if (ev.type == KEYEVENT_KEYDOWN || ev.type == KEYEVENT_RAWKEYDOWN) {
						if (ev.modifiers & EVENTFLAG_ALT_DOWN) {
							if (ev.windows_key_code == 0x25 /* VK_LEFT */) {
								view->goBack();
								return true;
							} else if (ev.windows_key_code == 0x27 /* VK_RIGHT */) {
								view->goForward();
								return true;
							}
						}
						if (ev.modifiers & EVENTFLAG_CONTROL_DOWN) {
							if (ev.windows_key_code == 0xBB /* VK_OEM_PLUS */ || ev.windows_key_code == 0x6B /* VK_ADD */) {
								view->zoomIn();
							} else if (ev.windows_key_code == 0xBD /* VK_OEM_MINUS */ || ev.windows_key_code == 0x6D /* VK_SUBTRACT */) {
								view->zoomOut();
							}
						}
					}
					return false;
				}

			private:
				IMPLEMENT_REFCOUNTING(ChromiumHandler);
			};
			
			ChromiumViewInstance::ChromiumViewInstance()
			{
				m_flagLoadOffline = sl_false;

				m_flagResizeAfterCreate = sl_false;
				m_flagReloadAfterCreate = sl_false;
			}
			
			ChromiumViewInstance::~ChromiumViewInstance()
			{
				closeBrowsers();
			}
			
			
			void ChromiumViewHelper::apply(ChromiumViewInstance* instance, CefWindowInfo& windowInfo)
			{
				StaticContext* context = GetStaticContext();
				if (!context) {
					return;
				}
				instance->m_flagLoadOffline = m_flagOfflineContent;
				CefBrowserSettings browser_settings;
				CefBrowserHost::CreateBrowser(windowInfo, context->handler, GetCefString(m_urlOrigin), browser_settings, NULL, NULL);
			}
			
			
			StaticContext::StaticContext(): app(new ChromiumApp), handler(new ChromiumHandler)
			{
				app->m_context = this;
				handler->m_context = this;
			}
			
			StaticContext::~StaticContext()
			{
			}
			
			static void CloseBrowsers()
			{
#ifndef SLIB_UI_IS_MACOS
				if (!CefCurrentlyOn(TID_UI)) {
					CefPostTask(TID_UI, base::Bind(&CloseBrowsers));
					return;
				}
#endif
				StaticContext* context = GetStaticContext();
				if (!context) {
					return;
				}
				static sl_bool flagRun = sl_false;
				if (flagRun) {
					return;
				}
				flagRun = sl_true;
				ListElements< CefRefPtr<CefBrowser> > browsers(context->browsers.duplicate());
				for (sl_size i = 0; i < browsers.count; i++) {
					CefRefPtr<CefBrowserHost> host = browsers[i]->GetHost();
					if (host.get()) {
						host->CloseBrowser(sl_true);
					}
				}
			}
			
#ifdef SLIB_UI_IS_MACOS
			static void ChromiumRunLoop()
			{
				CefRunMessageLoop();
			}
			
			static void QuitApp()
			{
				StaticContext* context = GetStaticContext();
				if (!context) {
					return;
				}
				CloseBrowsers();
				static int nTry = 0;
				nTry++;
				if (context->browsers.isNotEmpty() && nTry < 20) {
					dispatch_time_t t = dispatch_time(DISPATCH_TIME_NOW, 100 * NSEC_PER_MSEC);
					dispatch_after(t, dispatch_get_main_queue(), ^{
						QuitApp();
					});
				} else {
					CefQuitMessageLoop();
				}
			}
#endif
			
			static void Startup(CefMainArgs& args, const ChromiumSettings& _settings)
			{
				
				printf("Starting Chromium Embeded Framework\n");
				
#ifdef SLIB_UI_IS_MACOS
				[SLIBChromiumApplication sharedApplication];
				ui_core::SetCustomMessageLoop(&ChromiumRunLoop);
				ui_core::SetCustomQuitApp(&QuitApp);
				
				CefScopedLibraryLoader library_loader;
				if (File::exists(Apple::getMainBundlePath() + "/Contents/Frameworks/Chromium Embedded Framework.framework")) {
					if (!(library_loader.LoadInMain())) {
						exit(1);
						return;
					}
				} else {
					if (!(library_loader.LoadInHelper())) {
						exit(1);
						return;
					}
				}
#endif
				
				StaticContext* context = GetStaticContext();
				if (!context) {
					return;
				}
				
				// try to execute sub-processes (render, plugin, GPU, etc)
				int exit_code = CefExecuteProcess(args, context->app, NULL);
				if (exit_code >= 0) {
					// The sub-process has completed so return here.
					exit(exit_code);
					return;
				}
				
				CefEnableHighDPISupport();
				
				CefSettings settings;
				settings.no_sandbox = 1;
				if (_settings.userAgent.isNotEmpty()) {
					CefString(&(settings.user_agent)) = GetCefString(_settings.userAgent);
				}
				
#ifdef SLIB_UI_IS_MACOS
				{
					String exe = File::getFileNameOnly(Apple::getStringFromNSString([[NSBundle mainBundle] executablePath]));
					String path = String::join(Apple::getMainBundlePath(), "/Contents/Frameworks/", exe);
					if (File::exists(path + " Helper.app")) {
						path += " Helper.app";
						exe += " Helper";
					} else if (File::exists(path + "Helper.app")) {
						path += "Helper.app";
						exe += "Helper";
					} else {
						printf("Chromium Helper App is not found at: %sHelper.app\n", path.getData());
						exit(1);
					}
					path += "/Contents/MacOS/" + exe;
					if (File::exists(path)) {
						CefString(&(settings.browser_subprocess_path)) = GetCefString(path);
					} else {
						printf("Chromium Helper Executable is not found at: %s\n", path.getData());
						exit(1);
					}
				}
#else
				settings.multi_threaded_message_loop = 1;
#endif
				
				CefInitialize(args, settings, context->app, NULL);
				
			}
			
			static void Shutdown()
			{
#ifndef SLIB_UI_IS_MACOS
				StaticContext* context = GetStaticContext();
				if (!context) {
					return;
				}
				CloseBrowsers();
				for (int i = 0; i < 20; i++) {
					if (context->browsers.isEmpty()) {
						break;
					}
					Thread::sleep(100);
				}
#endif
				CefShutdown();
			}
			
			class CookieVisitor : public CefCookieVisitor
			{
			public:
				List<HttpCookie> cookies;
				Function<void(const List<HttpCookie>& cookies)> callback;
				
			public:
				bool Visit(const CefCookie& cookie, int count, int total, bool& deleteCookie) override
				{
					HttpCookie h;
					h.name = GetString(CefString(&(cookie.name)));
					h.value = GetString(CefString(&(cookie.value)));
					h.domain = GetString(CefString(&(cookie.domain)));
					h.path = GetString(CefString(&(cookie.path)));
					h.secure = cookie.secure != 0;
					h.http_only = cookie.httponly != 0;
					if (cookie.has_expires) {
						cef_time_t const& t = cookie.expires;
						h.expires = Time(t.year, t.month, t.day_of_month, t.hour, t.minute, t.second, t.millisecond);
					}
					cookies.add_NoLock(h);
					if (count + 1 == total) {
						callback(cookies);
					}
					return sl_true;
				}
				
			private:
				IMPLEMENT_REFCOUNTING(CookieVisitor);
			};
			
			class SetCookieCallback : public CefSetCookieCallback
			{
			public:
				Function<void(sl_bool)> callback;
				
			public:
				void OnComplete(bool success) override
				{
					callback(success);
				}
				
			private:
				IMPLEMENT_REFCOUNTING(SetCookieCallback);
			};
			
			class DeleteCookiesCallback : public CefDeleteCookiesCallback
			{
			public:
				Function<void(sl_uint32)> callback;
				
			public:
				void OnComplete(int num_deleted) override
				{
					callback(num_deleted);
				}
				
			private:
				IMPLEMENT_REFCOUNTING(DeleteCookiesCallback);
			};
			
		}
	}
	
	using namespace priv::chromium;
	
	Ref<ViewInstance> ChromiumView::createNativeWidget(ViewInstance* parent)
	{
		StaticContext* context = GetStaticContext();
		if (!context) {
			return sl_null;
		}
#if defined(SLIB_PLATFORM_IS_WIN32)
		Win32_UI_Shared* shared = Win32_UI_Shared::get();
		if (!shared) {
			return sl_null;
		}
		return Win32_ViewInstance::create<ChromiumViewInstance>(this, parent, (LPCWSTR)((LONG_PTR)(shared->wndClassForView)), sl_null, 0, 0);
#elif defined(SLIB_PLATFORM_IS_MACOS)
		return macOS_ViewInstance::create<ChromiumViewInstance, SLIBViewHandle>(this, parent);
#else
		return sl_null;
#endif
	}
	
	Ptr<IWebViewInstance> ChromiumView::getWebViewInstance()
	{
		return CastRef<ChromiumViewInstance>(getViewInstance());
	}
	
	
	SLIB_DEFINE_CLASS_DEFAULT_MEMBERS(ChromiumSettings)
	
	ChromiumSettings::ChromiumSettings()
	{
	}
	
#ifdef SLIB_PLATFORM_IS_WIN32
	void Chromium::startup(void* hInstance)
	{
		startup(hInstance, ChromiumSettings());
	}
#else
	void Chromium::startup(int argc, const void* argv)
	{
		startup(argc, argv, ChromiumSettings());
	}
#endif
	
#ifdef SLIB_PLATFORM_IS_WIN32
	void Chromium::startup(void* hInstance, const ChromiumSettings& settings)
#else
	void Chromium::startup(int argc, const void* argv, const ChromiumSettings& settings)
#endif
	{
#ifdef SLIB_PLATFORM_IS_WIN32
		CefMainArgs args((HINSTANCE)hInstance);
#else
		CefMainArgs args(argc, (char**)argv);
#endif
		Startup(args, settings);
	}
	
	void Chromium::shutdown()
	{
		Shutdown();
	}
	
	void Chromium::clearCache()
	{
	}
	
	void Chromium::getAllCookies(const Function<void(const List<HttpCookie>& cookies)>& callback)
	{
		CefRefPtr<CefCookieManager> refManager = CefCookieManager::GetGlobalManager(nullptr);
		CefCookieManager* manager = refManager.get();
		if (manager) {
			CefRefPtr<CookieVisitor> refVisitor = new CookieVisitor;
			CookieVisitor* visitor = refVisitor.get();
			if (visitor) {
				visitor->callback = callback;
				manager->VisitAllCookies(refVisitor);
				return;
			}
		}
		callback(sl_null);
	}
	
	void Chromium::getAllCookies(const String& url, const Function<void(const List<HttpCookie>& cookies)>& callback)
	{
		CefRefPtr<CefCookieManager> refManager = CefCookieManager::GetGlobalManager(nullptr);
		CefCookieManager* manager = refManager.get();
		if (manager) {
			CefRefPtr<CookieVisitor> refVisitor = new CookieVisitor;
			CookieVisitor* visitor = refVisitor.get();
			if (visitor) {
				visitor->callback = callback;
				manager->VisitUrlCookies(GetCefString(url), sl_true, refVisitor);
				return;
			}
		}
		callback(sl_null);
	}
	
	void Chromium::setCookie(const String& url, const HttpCookie& h, const Function<void(sl_bool flagSuccess)>& callback)
	{
		CefRefPtr<CefCookieManager> refManager = CefCookieManager::GetGlobalManager(nullptr);
		CefCookieManager* manager = refManager.get();
		if (manager) {
			CefRefPtr<SetCookieCallback> refWrapperCallback = new SetCookieCallback;
			SetCookieCallback* wrapperCallback = refWrapperCallback.get();
			if (wrapperCallback) {
				wrapperCallback->callback = callback;
				CefCookie cookie;
				CefString(&(cookie.name)) = GetCefString(h.name);
				CefString(&(cookie.value)) = GetCefString(h.value);
				CefString(&(cookie.domain)) = GetCefString(h.domain);
				CefString(&(cookie.path)) = GetCefString(h.path);
				cookie.secure = h.secure ? 1 : 0;
				cookie.httponly = h.http_only ? 1 : 0;
				if (h.expires.isNotZero()) {
					cookie.has_expires = true;
					Time const& t = h.expires;
					cookie.expires.year = t.getYear();
					cookie.expires.month = t.getMonth();
					cookie.expires.day_of_month = t.getDay();
					cookie.expires.day_of_week = t.getDayOfWeek();
					cookie.expires.hour = t.getHour();
					cookie.expires.minute = t.getMinute();
					cookie.expires.second = t.getSecond();
					cookie.expires.millisecond = t.getMillisecond();
				} else {
					cookie.has_expires = false;
				}
				manager->SetCookie(GetCefString(url), cookie, refWrapperCallback);
				return;
			}
		}
		callback(sl_false);
	}
	
	void Chromium::deleteCookies(const String& url, const String& name, const Function<void(sl_uint32 countDeleted)>& callback)
	{
		CefRefPtr<CefCookieManager> refManager = CefCookieManager::GetGlobalManager(nullptr);
		CefCookieManager* manager = refManager.get();
		if (manager) {
			CefRefPtr<DeleteCookiesCallback> refWrapperCallback = new DeleteCookiesCallback;
			DeleteCookiesCallback* wrapperCallback = refWrapperCallback.get();
			if (wrapperCallback) {
				wrapperCallback->callback = callback;
				manager->DeleteCookies(GetCefString(url), GetCefString(name), refWrapperCallback);
				return;
			}
		}
		callback(0);
	}
	
	void Chromium::clearCookies()
	{
		CefRefPtr<CefCookieManager> refManager = CefCookieManager::GetGlobalManager(nullptr);
		CefCookieManager* manager = refManager.get();
		if (manager) {
			manager->FlushStore(nullptr);
		}
	}
	
#else
	
	Ref<ViewInstance> ChromiumView::createNativeWidget(ViewInstance* parent)
	{
		return WebView::createNativeWidget(parent);
	}
	
	Ptr<IWebViewInstance> ChromiumView::getWebViewInstance()
	{
		return WebView::getWebViewInstance();
	}
	
	void Chromium::startup(int argc, const void* argv)
	{
	}
	
	void Chromium::startup(int argc, const void* argv, const ChromiumSettings& settings)
	{
	}
	
	void Chromium::shutdown()
	{
	}
	
	void Chromium::clearCache()
	{
	}
	
	void Chromium::getAllCookies(const Function<void(const List<HttpCookie>& cookies)>& callback)
	{
		callback(sl_null);
	}
	
	void Chromium::getAllCookies(const String& url, const Function<void(const List<HttpCookie>& cookies)>& callback)
	{
		callback(sl_null);
	}
	
	void Chromium::setCookie(const String& url, const HttpCookie& cookie, const Function<void(sl_bool flagSuccess)>& callback)
	{
		callback(sl_false);
	}
	
	void Chromium::deleteCookies(const String& url, const String& name, const Function<void(sl_uint32 countDeleted)>& callback)
	{
		callback(0);
	}
	
	void Chromium::clearCookies()
	{
	}
	
#endif
	
}

#ifdef SLIB_UI_IS_MACOS

using namespace slib;
using namespace slib::priv::chromium;

@implementation SLIBChromiumApplication

- (BOOL)isHandlingSendEvent
{
	return handlingSendEvent_;
}

- (void)setHandlingSendEvent:(BOOL)handlingSendEvent
{
	handlingSendEvent_ = handlingSendEvent;
}

- (void)sendEvent:(NSEvent*)event
{
	CefScopedSendingEvent sendingEventScoper;
	[super sendEvent:event];
}

- (void)terminate:(id)sender
{
	CloseBrowsers();
}

@end

#endif
