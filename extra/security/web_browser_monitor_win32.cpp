#include <slib/core/definition.h>

#ifdef SLIB_PLATFORM_IS_WIN32

#include "web_browser_monitor.h"

#include <slib/core/variant.h>
#include <slib/core/queue.h>
#include <slib/core/thread.h>
#include <slib/core/safe_static.h>
#include <slib/platform/win32/platform.h>
#include <slib/platform/win32/com.h>

#include <objbase.h>
#include <uiautomation.h>

#define CHROME_WINDOW_CLASS L"Chrome_WidgetWin_1"
#define FIREFOX_WINDOW_CLASS L"MozillaWindowClass"
#define IE_WINDOW_CLASS L"IEFrame"

namespace slib
{

	namespace
	{

		template <class INTERFACE>
		using ComPtr = win32::ComPtr<INTERFACE>;

		static ComPtr<IUIAutomationCondition> CreateCondition(IUIAutomation* automation, PROPERTYID propId, const VARIANT& value)
		{
			IUIAutomationCondition* condition = NULL;
			automation->CreatePropertyCondition(propId, value, &condition);
			return condition;
		}

		static ComPtr<IUIAutomationCondition> CreateCondition(IUIAutomation* automation, PROPERTYID propId, INT value)
		{
			VARIANT var = { 0 };
			var.vt = VT_INT;
			var.intVal = value;
			return CreateCondition(automation, propId, var);
		}

		static ComPtr<IUIAutomationCondition> CreateCondition(IUIAutomation* automation, PROPERTYID propId, BSTR value)
		{
			VARIANT var = { 0 };
			var.vt = VT_BSTR;
			var.bstrVal = SysAllocString(value);
			ComPtr<IUIAutomationCondition> condition = CreateCondition(automation, propId, var);
			VariantClear(&var);
			return condition;
		}

		static ComPtr<IUIAutomationCondition> CreateOrCondition(IUIAutomation* automation, ComPtr<IUIAutomationCondition>&& condition1, ComPtr<IUIAutomationCondition>&& condition2, ComPtr<IUIAutomationCondition>&& condition3)
		{
			IUIAutomationCondition* condition = NULL;
			if (condition1.isNotNull()) {
				if (condition2.isNotNull()) {
					if (condition3.isNotNull()) {
						IUIAutomationCondition* conditions[] = { condition1.get(), condition2.get(), condition3.get() };
						automation->CreateOrConditionFromNativeArray(conditions, 3, &condition);
						condition3.setNull();
					}
					condition2.setNull();
				}
				condition1.setNull();
			}
			return condition;
		}

		static ComPtr<IUIAutomationCondition> CreateAndCondition(IUIAutomation* automation, ComPtr<IUIAutomationCondition>&& condition1, ComPtr<IUIAutomationCondition>&& condition2)
		{
			IUIAutomationCondition* condition = NULL;
			if (condition1.isNotNull()) {
				if (condition2.isNotNull()) {
					automation->CreateAndCondition(condition1.get(), condition2.get(), &condition);
					condition2.setNull();
				}
				condition1.setNull();
			}
			return condition;
		}

		static ComPtr<IUIAutomationElement> FindElement(IUIAutomationElement* parent, TreeScope scope, ComPtr<IUIAutomationCondition>&& condition)
		{
			IUIAutomationElement* ret = NULL;
			if (condition.isNotNull()) {
				parent->FindFirst(scope, condition.get(), &ret);
				condition.setNull();
			}
			return ret;
		}

		static ComPtr<IUIAutomationElementArray> FindElements(IUIAutomationElement* parent, TreeScope scope, ComPtr<IUIAutomationCondition>&& condition)
		{
			IUIAutomationElementArray* ret = NULL;
			if (condition.isNotNull()) {
				parent->FindAll(scope, condition.get(), &ret);
				condition.setNull();
			}
			return ret;
		}

		static ComPtr<IUIAutomationElement> FindEditElementByName(IUIAutomation* automation, IUIAutomationElement* parent, TreeScope scope, BSTR name)
		{
			return FindElement(parent, scope, CreateAndCondition(automation, CreateCondition(automation, UIA_ControlTypePropertyId, UIA_EditControlTypeId), CreateCondition(automation, UIA_NamePropertyId, name)));
		}

		static ComPtr<IUIAutomationElement> FindEditElementById(IUIAutomation* automation, IUIAutomationElement* parent, TreeScope scope, BSTR id)
		{
			return FindElement(parent, scope, CreateAndCondition(automation, CreateCondition(automation, UIA_ControlTypePropertyId, UIA_EditControlTypeId), CreateCondition(automation, UIA_AutomationIdPropertyId, id)));
		}

		static Variant GetElementProperty(IUIAutomationElement* element, PROPERTYID propId)
		{
			Variant ret;
			VARIANT var = { 0 };
			HRESULT hr = element->GetCurrentPropertyValue(propId, &var);
			if (SUCCEEDED(hr)) {
				ret = Win32::getVariantFromVARIANT(&var);
			}
			VariantClear(&var);
			return ret;
		}

		static sl_size GetElementWindowHandle(IUIAutomationElement* element)
		{
			UIA_HWND hWnd = NULL;
			element->get_CurrentNativeWindowHandle(&hWnd);
			return (sl_size)hWnd;
		}

		static String16 GetElementClassName(IUIAutomationElement* element)
		{
			BSTR str = NULL;
			element->get_CurrentClassName(&str);
			String16 ret = String16::from(str);
			if (str) {
				SysFreeString(str);
			}
			return ret;
		}

		static String GetElementName(IUIAutomationElement* element)
		{
			BSTR str = NULL;
			element->get_CurrentName(&str);
			String ret = String::from(str);
			if (str) {
				SysFreeString(str);
			}
			return ret;
		}

		static ComPtr<IUIAutomationElement> FindAddressBarElement(IUIAutomation* automation, IUIAutomationElement* element)
		{
			String16 className = GetElementClassName(element);
			if (className == (sl_char16*)CHROME_WINDOW_CLASS) {
				return FindEditElementByName(automation, element, TreeScope_Subtree, L"Address and search bar");
			} else if (className == (sl_char16*)FIREFOX_WINDOW_CLASS) {
				return FindEditElementById(automation, element, TreeScope_Subtree, L"urlbar-input");
			} else if (className == (sl_char16*)IE_WINDOW_CLASS) {
				return FindEditElementByName(automation, element, TreeScope_Subtree, L"Address");
			} else {
				return sl_null;
			}
		}

		static String GetAddressBarText(IUIAutomation* automation, IUIAutomationElement* element)
		{
			ComPtr<IUIAutomationElement> addressBar = FindAddressBarElement(automation, element);
			if (addressBar.isNull()) {
				return sl_null;
			}
			return GetElementProperty(addressBar.get(), UIA_ValueValuePropertyId).getString();
		}

		static String GetBrowserTitle(IUIAutomation* automation, IUIAutomationElement* element)
		{
			ComPtr<IUIAutomationElement> pane = FindElement(element, TreeScope_Children, CreateCondition(automation, UIA_ClassNamePropertyId, L"BrowserRootView"));
			if (pane.isNotNull()) {
				// Edge
				return GetElementName(pane.get());
			} else {
				return GetElementName(element);
			}
		}

		static sl_bool IsBrowserElement(IUIAutomation* automation, IUIAutomationElement* element)
		{
			String16 name = GetElementClassName(element);
			return name == (sl_char16*)CHROME_WINDOW_CLASS || name == (sl_char16*)FIREFOX_WINDOW_CLASS || name == (sl_char16*)IE_WINDOW_CLASS;
		}

		static ComPtr<IUIAutomationElementArray> FindBrowserElements(IUIAutomation* automation, IUIAutomationElement* root)
		{
			return FindElements(root, TreeScope_Children, CreateOrCondition(automation, CreateCondition(automation, UIA_ClassNamePropertyId, CHROME_WINDOW_CLASS), CreateCondition(automation, UIA_ClassNamePropertyId, FIREFOX_WINDOW_CLASS), CreateCondition(automation, UIA_ClassNamePropertyId, IE_WINDOW_CLASS)));
		}

		class MonitorContext
		{
		public:
			Mutex lock;
			Ref<Thread> thread;

			CList< Function<void(WebBrowserMonitor::Page&)> > callbacks;
			AtomicFunction<void(WebBrowserMonitor::Page&)> mergedCallback;

		public:
			MonitorContext()
			{
			}

			~MonitorContext()
			{
				if (thread.isNotNull()) {
					thread->finishAndWait();
				}
			}

		public:
			void addCallback(const Function<void(WebBrowserMonitor::Page&)>& callback)
			{
				MutexLocker locker(&lock);
				callbacks.addIfNotExist_NoLock(callback);
				updateCallback();
				if (thread.isNull()) {
					thread = Thread::start(SLIB_FUNCTION_MEMBER(this, run));
				}
			}

			void removeCallback(const Function<void(WebBrowserMonitor::Page&)>& callback)
			{
				MutexLocker locker(&lock);
				callbacks.remove_NoLock(callback);
				updateCallback();
				Ref<Thread> oldThread = Move(thread);
				locker.unlock();
				if (oldThread.isNotNull()) {
					oldThread->finishAndWait();
				}
			}

			void updateCallback()
			{
				ListLocker< Function<void(WebBrowserMonitor::Page&)> > list(callbacks);
				if (list.count == 1) {
					mergedCallback = list[0];
				} else {
					Function<void(WebBrowserMonitor::Page&)> callback;
					for (sl_size i = 0; i < list.count; i++) {
						callback.add(list[i]);
					}
					mergedCallback = Move(callback);
				}
			}

			void run();

		};

		class MonitorThread : public IUIAutomationEventHandler, public IUIAutomationPropertyChangedEventHandler
		{
		public:
			MonitorContext* context;

			ComPtr<IUIAutomation> automation;
			Ref<Event> event;
			Queue< Pair<ComPtr<IUIAutomationElement>, EVENTID> > queueEvents;
			Queue<sl_size> queueRemovingWindows;
			HashMap< sl_size, ComPtr<IUIAutomationElement> > watchingBrowsers;
			HashMap<sl_size, String> lastTitles;

		public:
			void onUpdate(sl_size id, String& url, String& title)
			{
				String lastTitle = lastTitles.getValue_NoLock(id);
				if (lastTitle == title) {
					return;
				}
				lastTitles.put_NoLock(id, title);
				WebBrowserMonitor::Page page;
				page.title = title;
				page.url = url;
				page.windowHandle = id;
				context->mergedCallback(page);
			}

			void onUpdate(sl_size id, IUIAutomationElement* element)
			{
				String title = GetBrowserTitle(automation.get(), element);
				if (title.isNotEmpty()) {
					String url = GetAddressBarText(automation.get(), element);
					if (url.isNotEmpty()) {
						onUpdate(id, url, title);
					}
				}
			}

			void addEventHandler(IUIAutomationElement* element)
			{
				sl_size id = GetElementWindowHandle(element);
				if (!id) {
					return;
				}
				PROPERTYID propId = UIA_NamePropertyId;
				automation->AddPropertyChangedEventHandlerNativeArray(element, TreeScope_Element, NULL, this, &propId, 1);
				ComPtr<IUIAutomationElement> addressBar = FindAddressBarElement(automation, element);
				if (addressBar.isNotNull()) {
					if (!(watchingBrowsers.find_NoLock(id))) {
						if (watchingBrowsers.put_NoLock(id, element)) {
							element->AddRef();
						}
					}
				}
			}

			void addEventHandlers(IUIAutomationElement* root)
			{
				ComPtr<IUIAutomationElementArray> arr = FindBrowserElements(automation.get(), root);
				if (arr.isNotNull()) {
					int n = 0;
					arr->get_Length(&n);
					for (int i = 0; i < n; i++) {
						ComPtr<IUIAutomationElement> element;
						arr->GetElement(i, &(element.ptr));
						if (element.isNotNull()) {
							ComPtr<IUIAutomationElement> addressBar = FindAddressBarElement(automation, element);
							if (addressBar.isNotNull()) {
								addEventHandler(element.get());
							}
						}
					}
				}
			}

			void processEvent(IUIAutomationElement* element, EVENTID eventId)
			{
				if (eventId == UIA_Window_WindowOpenedEventId) {
					addEventHandler(element);
				} else if (eventId == UIA_AutomationPropertyChangedEventId) {
					sl_size id = GetElementWindowHandle(element);
					if (IsWindow((HWND)id)) {
						watchingBrowsers.remove_NoLock(id);
						onUpdate(id, element);
					}
				}
			}

			void processEvents()
			{
				Pair<ComPtr<IUIAutomationElement>, EVENTID> item;
				while (queueEvents.pop(&item)) {
					processEvent(item.first, item.second);
				}
				sl_uint64 id;
				while (queueRemovingWindows.pop(&id)) {
					watchingBrowsers.remove_NoLock(id);
					lastTitles.remove_NoLock(id);
				}
			}

			void run()
			{
				event = Event::create();
				if (event.isNull()) {
					return;
				}
				CoInitializeEx(NULL, COINIT_MULTITHREADED);
				if (automation.createInstance(CLSID_CUIAutomation)) {
					ComPtr<IUIAutomationElement> root;
					automation->GetRootElement(&(root.ptr));
					if (root.isNotNull()) {
						automation->AddAutomationEventHandler(UIA_Window_WindowOpenedEventId, root.get(), TreeScope_Children, NULL, this);
						automation->AddAutomationEventHandler(UIA_Window_WindowClosedEventId, root.get(), TreeScope_Children, NULL, this);
						addEventHandlers(root.get());
						CurrentThread thread;
						while (thread.isNotStopping()) {
							processEvents();
							if (watchingBrowsers.isNotEmpty()) {
								for (auto&& item : watchingBrowsers) {
									if (IsWindow((HWND)((void*)((sl_size)(item.key))))) {
										onUpdate(item.key, item.value.get());
									} else {
										queueRemovingWindows.push(item.key);
									}
								}
								event->wait(300);
							} else {
								event->wait();
							}
						}
						watchingBrowsers.setNull();
						automation->RemoveAllEventHandlers();
					}
					automation.setNull();
				}
				CoUninitialize();
			}

		public:
			// IUnknown methods.
			ULONG STDMETHODCALLTYPE AddRef() override
			{
				return 2;
			}

			ULONG STDMETHODCALLTYPE Release() override
			{
				return 1;
			}

			HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppInterface) override
			{
				if (riid == __uuidof(IUnknown) || riid == __uuidof(IUIAutomationEventHandler)) {
					*ppInterface = static_cast<IUIAutomationEventHandler*>(this);
					AddRef();
					return S_OK;
				} else if (riid == __uuidof(IUIAutomationPropertyChangedEventHandler)) {
					*ppInterface = static_cast<IUIAutomationPropertyChangedEventHandler*>(this);
					AddRef();
					return S_OK;
				} else {
					*ppInterface = NULL;
					return E_NOINTERFACE;
				}
			}

			// IUIAutomationEventHandler methods
			HRESULT STDMETHODCALLTYPE HandleAutomationEvent(IUIAutomationElement* sender, EVENTID eventId) override
			{
				if (eventId == UIA_Window_WindowOpenedEventId) {
					if (IsBrowserElement(automation, sender)) {
						sender->AddRef();
						queueEvents.push(sender, eventId);
						event->set();
					}
				} else if (eventId == UIA_Window_WindowClosedEventId) {
					sl_size id = GetElementWindowHandle(sender);
					queueRemovingWindows.push(id);
					event->set();
				}
				return S_OK;
			}

			HRESULT STDMETHODCALLTYPE HandlePropertyChangedEvent(IUIAutomationElement* sender, PROPERTYID propertyId, VARIANT newValue)
			{
				if (propertyId == UIA_NamePropertyId) {
					sender->AddRef();
					String s = GetElementName(sender);
					queueEvents.push(sender, UIA_AutomationPropertyChangedEventId);
					event->set();
				}
				return S_OK;
			}

		};

		void MonitorContext::run()
		{
			MonitorThread thread;
			thread.context = this;
			thread.run();
		}

		SLIB_SAFE_STATIC_GETTER(MonitorContext, GetMonitorContext)

	}

	List<WebBrowserMonitor::Page> WebBrowserMonitor::getCurrentPages()
	{
		List<WebBrowserMonitor::Page> ret;
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
		ComPtr<IUIAutomation> automation;
		if (automation.createInstance(CLSID_CUIAutomation)) {
			ComPtr<IUIAutomationElement> root;
			automation->GetRootElement(&(root.ptr));
			if (root.isNotNull()) {
				ComPtr<IUIAutomationElementArray> arr = FindBrowserElements(automation.get(), root.get());
				if (arr.isNotNull()) {
					int n = 0;
					arr->get_Length(&n);
					for (int i = 0; i < n; i++) {
						ComPtr<IUIAutomationElement> element;
						arr->GetElement(i, &(element.ptr));
						if (element.isNotNull()) {
							String title = GetBrowserTitle(automation.get(), element.get());
							if (title.isNotEmpty()) {
								String url = GetAddressBarText(automation.get(), element.get());
								if (url.isNotEmpty()) {
									WebBrowserMonitor::Page page;
									page.title = Move(title);
									page.url = Move(url);
									page.windowHandle = GetElementWindowHandle(element);
									ret.add_NoLock(Move(page));
								}
							}
						}
					}
				}
			}
		}
		return ret;
	}

	void WebBrowserMonitor::addMonitor(const Function<void(Page&)>& callback)
	{
		MonitorContext* context = GetMonitorContext();
		if (!context) {
			return;
		}
		context->addCallback(callback);
	}

	void WebBrowserMonitor::removeMonitor(const Function<void(Page&)>& callback)
	{
		MonitorContext* context = GetMonitorContext();
		if (!context) {
			return;
		}
		context->removeCallback(callback);
	}

}

#endif