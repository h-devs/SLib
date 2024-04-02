#include <slib/core/definition.h>

#ifdef SLIB_PLATFORM_IS_WIN32

#include "web_browser_monitor.h"

#include <slib/core/variant.h>
#include <slib/core/queue.h>
#include <slib/core/thread.h>
#include <slib/core/safe_static.h>
#include <slib/ui/platform.h>

#include <objbase.h>
#include <uiautomation.h>

#define CHROME_WINDOW_CLASS L"Chrome_WidgetWin_1"
#define FIREFOX_WINDOW_CLASS L"MozillaWindowClass"
#define IE_WINDOW_CLASS L"IEFrame"

namespace slib
{

	namespace
	{

		static IUIAutomationCondition* CreateCondition(IUIAutomation* automation, PROPERTYID propId, const VARIANT& value)
		{
			IUIAutomationCondition* condition = NULL;
			automation->CreatePropertyCondition(propId, value, &condition);
			return condition;
		}

		static IUIAutomationCondition* CreateCondition(IUIAutomation* automation, PROPERTYID propId, INT value)
		{
			VARIANT var = { 0 };
			var.vt = VT_INT;
			var.intVal = value;
			return CreateCondition(automation, propId, var);
		}

		static IUIAutomationCondition* CreateCondition(IUIAutomation* automation, PROPERTYID propId, BSTR value)
		{
			VARIANT var = { 0 };
			var.vt = VT_BSTR;
			var.bstrVal = SysAllocString(value);
			IUIAutomationCondition* condition = CreateCondition(automation, propId, var);
			VariantClear(&var);
			return condition;
		}

		static IUIAutomationCondition* CreateOrCondition(IUIAutomation* automation, IUIAutomationCondition*&& condition1, IUIAutomationCondition*&& condition2, IUIAutomationCondition*&& condition3)
		{
			IUIAutomationCondition* condition = NULL;
			if (condition1) {
				if (condition2) {
					if (condition3) {
						IUIAutomationCondition* conditions[] = { condition1, condition2, condition3 };
						automation->CreateOrConditionFromNativeArray(conditions, 3, &condition);
						condition3->Release();
						condition3 = NULL;
					}
					condition2->Release();
					condition2 = NULL;
				}
				condition1->Release();
				condition1 = NULL;
			}
			return condition;
		}

		static IUIAutomationCondition* CreateAndCondition(IUIAutomation* automation, IUIAutomationCondition*&& condition1, IUIAutomationCondition*&& condition2)
		{
			IUIAutomationCondition* condition = NULL;
			if (condition1) {
				if (condition2) {
					automation->CreateAndCondition(condition1, condition2, &condition);
					condition2->Release();
					condition2 = NULL;
				}
				condition1->Release();
				condition1 = NULL;
			}
			return condition;
		}

		static IUIAutomationElement* FindElement(IUIAutomationElement* parent, TreeScope scope, IUIAutomationCondition*&& condition)
		{
			IUIAutomationElement* ret = NULL;
			if (condition) {
				parent->FindFirst(scope, condition, &ret);
				condition->Release();
				condition = NULL;
			}
			return ret;
		}

		static IUIAutomationElementArray* FindElements(IUIAutomationElement* parent, TreeScope scope, IUIAutomationCondition*&& condition)
		{
			IUIAutomationElementArray* ret = NULL;
			if (condition) {
				parent->FindAll(scope, condition, &ret);
				condition->Release();
				condition = NULL;
			}
			return ret;
		}

		static IUIAutomationElement* FindEditElementByName(IUIAutomation* automation, IUIAutomationElement* parent, TreeScope scope, BSTR name)
		{
			return FindElement(parent, scope, CreateAndCondition(automation, CreateCondition(automation, UIA_ControlTypePropertyId, UIA_EditControlTypeId), CreateCondition(automation, UIA_NamePropertyId, name)));
		}

		static IUIAutomationElement* FindEditElementById(IUIAutomation* automation, IUIAutomationElement* parent, TreeScope scope, BSTR id)
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

		static String GetAddressBarText(IUIAutomation* automation, IUIAutomationElement* parent)
		{
			IUIAutomationElement* addressBar;
			String16 className = GetElementProperty(parent, UIA_ClassNamePropertyId).getString16();
			if (className == (sl_char16*)CHROME_WINDOW_CLASS) {
				addressBar = FindEditElementByName(automation, parent, TreeScope_Subtree, L"Address and search bar");
			} else if (className == (sl_char16*)FIREFOX_WINDOW_CLASS) {
				addressBar = FindEditElementById(automation, parent, TreeScope_Subtree, L"urlbar-input");
			} else if (className == (sl_char16*)IE_WINDOW_CLASS) {
				addressBar = FindEditElementByName(automation, parent, TreeScope_Subtree, L"Address");
			} else {
				return sl_null;
			}
			if (!addressBar) {
				return sl_null;
			}
			String ret = GetElementProperty(addressBar, UIA_ValueValuePropertyId).getString();
			addressBar->Release();
			return ret;
		}

		static sl_bool IsBrowserElement(IUIAutomationElement* element)
		{
			String16 name = GetElementProperty(element, UIA_ClassNamePropertyId).getString16();
			return name == (sl_char16*)CHROME_WINDOW_CLASS || name == (sl_char16*)FIREFOX_WINDOW_CLASS || name == (sl_char16*)IE_WINDOW_CLASS;
		}

		static IUIAutomationElementArray* FindBrowserElements(IUIAutomation* automation, IUIAutomationElement* root)
		{
			return FindElements(root, TreeScope_Children, CreateOrCondition(automation, CreateCondition(automation, UIA_ClassNamePropertyId, CHROME_WINDOW_CLASS), CreateCondition(automation, UIA_ClassNamePropertyId, FIREFOX_WINDOW_CLASS), CreateCondition(automation, UIA_ClassNamePropertyId, IE_WINDOW_CLASS)));
		}

		class MonitorContext
		{
		public:
			Mutex lock;
			Ref<Thread> thread;

			CList< Function<void(String& url, String& title)> > callbacks;
			AtomicFunction<void(String& url, String& title)> mergedCallback;

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
			void addCallback(const Function<void(String& url, String& title)>& callback)
			{
				MutexLocker locker(&lock);
				callbacks.addIfNotExist_NoLock(callback);
				updateCallback();
				if (thread.isNull()) {
					thread = Thread::start(SLIB_FUNCTION_MEMBER(this, run));
				}
			}

			void removeCallback(const Function<void(String& url, String& title)>& callback)
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
				ListLocker< Function<void(String& url, String& title)> > list(callbacks);
				if (list.count == 1) {
					mergedCallback = list[0];
				} else {
					Function<void(String& url, String& title)> callback;
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

			Ref<Event> event;
			Queue< Pair<IUIAutomationElement*, EVENTID> > queueEvents;

		public:
			void addEventHandler(IUIAutomation* automation, IUIAutomationElement* element)
			{
				PROPERTYID propId = UIA_NamePropertyId;
				automation->AddPropertyChangedEventHandlerNativeArray(element, TreeScope_Element, NULL, this, &propId, 1);
			}

			void addEventHandlers(IUIAutomation* automation, IUIAutomationElement* root)
			{
				IUIAutomationElementArray* arr = FindBrowserElements(automation, root);
				if (arr) {
					int n = 0;
					arr->get_Length(&n);
					for (int i = 0; i < n; i++) {
						IUIAutomationElement* element = NULL;
						arr->GetElement(i, &element);
						if (element) {
							addEventHandler(automation, element);
							element->Release();
						}
					}
					arr->Release();
				}
			}

			void processEvent(IUIAutomation* automation, IUIAutomationElement* element, EVENTID eventId)
			{
				if (eventId == UIA_Window_WindowOpenedEventId) {
					addEventHandler(automation, element);
				} else if (eventId == UIA_Window_WindowClosedEventId) {
					automation->RemovePropertyChangedEventHandler(element, this);
				} else if (eventId == UIA_AutomationPropertyChangedEventId) {
					String title = GetElementProperty(element, UIA_NamePropertyId).getString();
					if (title.isNotEmpty()) {
						String url = GetAddressBarText(automation, element);
						if (url.isNotEmpty()) {
							context->mergedCallback(url, title);
						}
					}
				}
				element->Release();
			}

			void processEvents(IUIAutomation* automation)
			{
				Pair<IUIAutomationElement*, EVENTID> item;
				while (queueEvents.pop(&item)) {
					processEvent(automation, item.first, item.second);
				}
			}

			void run()
			{
				event = Event::create();
				if (event.isNull()) {
					return;
				}
				CoInitializeEx(NULL, COINIT_MULTITHREADED);
				IUIAutomation* automation = NULL;
				HRESULT hr = CoCreateInstance(CLSID_CUIAutomation, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&automation));
				if (automation) {
					IUIAutomationElement* root = NULL;
					automation->GetRootElement(&root);
					if (root) {
						hr = automation->AddAutomationEventHandler(UIA_Window_WindowOpenedEventId, root, TreeScope_Children, NULL, this);
						hr = automation->AddAutomationEventHandler(UIA_Window_WindowClosedEventId, root, TreeScope_Children, NULL, this);
						addEventHandlers(automation, root);
						CurrentThread thread;
						while (thread.isNotStopping()) {
							processEvents(automation);
							event->wait();
						}
						automation->RemoveAllEventHandlers();
						root->Release();
					}
					automation->Release();
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
				if (eventId == UIA_Window_WindowOpenedEventId || eventId == UIA_Window_WindowClosedEventId) {
					if (IsBrowserElement(sender)) {
						sender->AddRef();
						queueEvents.push(sender, eventId);
						event->set();
					}
				}
				return S_OK;
			}

			HRESULT STDMETHODCALLTYPE HandlePropertyChangedEvent(IUIAutomationElement* sender, PROPERTYID propertyId, VARIANT newValue)
			{
				if (propertyId == UIA_NamePropertyId) {
					sender->AddRef();
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
		IUIAutomation* automation = NULL;
		HRESULT hr = CoCreateInstance(CLSID_CUIAutomation, NULL, CLSCTX_INPROC, IID_PPV_ARGS(&automation));
		if (automation) {
			IUIAutomationElement* root = NULL;
			hr = automation->GetRootElement(&root);
			if (root) {
				IUIAutomationElementArray* arr = FindBrowserElements(automation, root);
				if (arr) {
					int n = 0;
					arr->get_Length(&n);
					for (int i = 0; i < n; i++) {
						IUIAutomationElement* element = NULL;
						arr->GetElement(i, &element);
						if (element) {
							String title = GetElementProperty(element, UIA_NamePropertyId).getString();
							if (title.isNotEmpty()) {
								String url = GetAddressBarText(automation, element);
								if (url.isNotEmpty()) {
									WebBrowserMonitor::Page page;
									page.title = Move(title);
									page.url = Move(url);
									ret.add_NoLock(Move(page));
								}
							}
							element->Release();
						}
					}
					arr->Release();
				}
				root->Release();
			}
			automation->Release();
		}
		return ret;
	}

	void WebBrowserMonitor::addMonitor(const Function<void(String& url, String& title)>& callback)
	{
		MonitorContext* context = GetMonitorContext();
		if (!context) {
			return;
		}
		context->addCallback(callback);
	}

	void WebBrowserMonitor::removeMonitor(const Function<void(String& url, String& title)>& callback)
	{
		MonitorContext* context = GetMonitorContext();
		if (!context) {
			return;
		}
		context->removeCallback(callback);
	}

}

#endif