/*
 *   Copyright (c) 2008-2020 SLIBIO <https://github.com/SLIBIO>
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

#include "slib/media/camera.h"

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/core/win32_com.h"
#include "slib/core/log.h"

#include <dshow.h>

EXTERN_C const IID IID_ISampleGrabberCB;

MIDL_INTERFACE("0579154A-2B53-4994-B0D0-E773148EFF85")
ISampleGrabberCB : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SampleCB(
		double SampleTime,
		IMediaSample *pSample) = 0;

	virtual HRESULT STDMETHODCALLTYPE BufferCB(
		double SampleTime,
		BYTE *pBuffer,
		long BufferLen) = 0;
};

EXTERN_C const IID IID_ISampleGrabber;

MIDL_INTERFACE("6B652FFF-11FE-4fce-92AD-0266B5D7C78F")
ISampleGrabber : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SetOneShot(
		BOOL OneShot) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetMediaType(
		const AM_MEDIA_TYPE *pType) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetConnectedMediaType(
		AM_MEDIA_TYPE *pType) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetBufferSamples(
		BOOL BufferThem) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetCurrentBuffer(
		/* [out][in] */ long *pBufferSize,
		/* [out] */ long *pBuffer) = 0;

	virtual HRESULT STDMETHODCALLTYPE GetCurrentSample(
		/* [retval][out] */ IMediaSample **ppSample) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetCallback(
		ISampleGrabberCB *pCallback,
		long WhichMethodToCallback) = 0;
};

EXTERN_C const CLSID CLSID_SampleGrabber;
EXTERN_C const CLSID CLSID_NullRenderer;

#pragma comment(lib, "strmiids.lib")

#define TAG "Camera"
#define LOG_ERROR(...) LogError(TAG, ##__VA_ARGS__)
#define LOG_HRESULT(...) LogHResult(__VA_ARGS__)

namespace slib
{

	namespace priv
	{
		namespace dshow
		{

			static void LogHResult(String error, HRESULT hr)
			{
				LOG_ERROR("%s (Result=%d)", error, (sl_int32)hr);
			}
			
			class CameraImpl : public Camera, public ISampleGrabberCB
			{
			public:
				ICaptureGraphBuilder2* m_capture;
				IGraphBuilder* m_graph;
				IMediaControl* m_control;
				ISampleGrabber* m_grabber;

				sl_bool m_flagRunning;

			public:
				CameraImpl()
				{
					m_capture = sl_null;
					m_graph = sl_null;
					m_control = sl_null;
					m_grabber = sl_null;

					m_flagRunning = sl_false;
				}

				~CameraImpl()
				{
					release();
				}

			public:
				static Ref<CameraImpl> _create(const CameraParam& param)
				{
					HRESULT hr;

					IBaseFilter* filterSource = NULL;
					_queryDevices(param.deviceId, &filterSource);

					if (filterSource) {

						IGraphBuilder* graph = NULL;
						hr = CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC, IID_IGraphBuilder, (void**)&graph);
						if (graph) {
							ICaptureGraphBuilder2* capture = NULL;
							hr = CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC, IID_ICaptureGraphBuilder2, (void**)&capture);
							if (capture) {
								IMediaControl* control = NULL;
								hr = graph->QueryInterface(IID_IMediaControl, (LPVOID*)&control);
								if (control) {
									IBaseFilter* filterGrabber = NULL;
									hr = CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&filterGrabber);
									if (filterGrabber) {
										IBaseFilter* filterNullRenderer = NULL;
										hr = CoCreateInstance(CLSID_NullRenderer, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&filterNullRenderer);
										if (filterNullRenderer) {

											hr = capture->SetFiltergraph(graph);
											if (SUCCEEDED(hr)) {
												hr = graph->AddFilter(filterSource, L"Video Capture");
												if (SUCCEEDED(hr)) {
													hr = graph->AddFilter(filterGrabber, L"Sample Grabber");
													if (SUCCEEDED(hr)) {
														ISampleGrabber* grabber = NULL;
														hr = filterGrabber->QueryInterface(IID_ISampleGrabber, (void**)&grabber);
														if (grabber) {
															AM_MEDIA_TYPE mt;
															ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
															mt.majortype = MEDIATYPE_Video;
															mt.subtype = MEDIASUBTYPE_RGB24;
															hr = grabber->SetMediaType(&mt);
															if (SUCCEEDED(hr)) {
																hr = graph->AddFilter(filterNullRenderer, L"Null Renderer");
																if (SUCCEEDED(hr)) {
																	hr = capture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, filterSource, filterGrabber, filterNullRenderer);
																	if (SUCCEEDED(hr)) {
																		Ref<CameraImpl> ret = new CameraImpl();
																		if (ret.isNotNull()) {
																			hr = grabber->SetCallback(ret.get(), 0);
																			if (SUCCEEDED(hr)) {
																				ret->m_capture = capture;
																				ret->m_graph = graph;
																				ret->m_control = control;
																				ret->m_grabber = grabber;
																				ret->_init(param);
																				if (param.flagAutoStart) {
																					ret->start();
																				}
																				return ret;
																			} else {
																				LOG_HRESULT("Failed to set capture callback", hr);
																			}
																		}
																	} else {
																		LOG_HRESULT("Failed to render capture stream", hr);
																	}
																} else {
																	LOG_HRESULT("Failed to add null rendering filter", hr);
																}
															} else {
																LOG_HRESULT("Failed to set grabber media type", hr);
															}
														} else {
															LOG_HRESULT("Failed to query sample grabber", hr);
														}
													} else {
														LOG_HRESULT("Failed to add sample grabber filter", hr);
													}
												} else {
													LOG_HRESULT("Failed to add source filter", hr);
												}
											} else {
												LOG_HRESULT("Failed to set FilterGraph", hr);
											}
											filterNullRenderer->Release();
										} else {
											LOG_HRESULT("Failed to create CLSID_NullRenderer", hr);
										}
										filterGrabber->Release();
									} else {
										LOG_HRESULT("Failed to create CLSID_SampleGrabber", hr);
									}
									control->Release();
								} else {
									LOG_HRESULT("Failed to query IMediaControl", hr);
								}
								capture->Release();
							} else {
								LOG_HRESULT("Failed to create CLSID_CaptureGraphBuilder2", hr);
							}
							graph->Release();
						} else {
							LOG_HRESULT("Failed to create CLSID_FilterGraph", hr);
						}

						filterSource->Release();

					} else {
						LOG_ERROR("Failed to find capture device: %s", param.deviceId);
					}

					return sl_null;
				}

				void release()
				{
					ObjectLocker lock(this);
					if (m_capture) {
						stop();
						SLIB_WIN32_COM_SAFE_RELEASE(m_capture);
						SLIB_WIN32_COM_SAFE_RELEASE(m_graph);
						SLIB_WIN32_COM_SAFE_RELEASE(m_control);
						SLIB_WIN32_COM_SAFE_RELEASE(m_grabber);
					}
				}

				sl_bool isOpened()
				{
					return m_capture != sl_null;
				}

				void start()
				{
					ObjectLocker lock(this);
					if (m_flagRunning) {
						return;
					}
					if (m_capture && m_control) {
						HRESULT hr = m_control->Run();
						if (SUCCEEDED(hr)) {
							m_flagRunning = sl_true;
						} else {
							OAFilterState fs;
							if (SUCCEEDED(m_control->GetState(10, &fs))) {
								LOG_HRESULT("Device is already in use", hr);
							} else {
								LOG_HRESULT("Failed to start capture", hr);
							}
						}
					}
				}

				void stop()
				{
					ObjectLocker lock(this);
					if (!m_flagRunning) {
						return;
					}
					if (m_capture && m_control) {
						m_control->Stop();
					}
					m_flagRunning = sl_false;
				}

				sl_bool isRunning()
				{
					return m_flagRunning;
				}

				HRESULT STDMETHODCALLTYPE BufferCB(double SampleTime, BYTE *pBuffer, long BufferLen) {
					return S_OK;
				}

				HRESULT STDMETHODCALLTYPE SampleCB(double SampleTime, IMediaSample *pSample) {

					HRESULT hr;

					long cbBuffer = 0;
					char* pBuffer = NULL;

					pSample->GetPointer((BYTE**)&pBuffer);
					cbBuffer = pSample->GetSize();
					if (!pBuffer) {
						return E_FAIL;
					}
					AM_MEDIA_TYPE mt;
					hr = m_grabber->GetConnectedMediaType(&mt);
					if (FAILED(hr)) {
						return hr;
					}

					VideoCaptureFrame frame;
					if (mt.majortype == MEDIATYPE_Video) {
						if (mt.formattype == FORMAT_VideoInfo && mt.cbFormat >= sizeof(VIDEOINFOHEADER)) {
							VIDEOINFOHEADER* vih = (VIDEOINFOHEADER*)(mt.pbFormat);
							frame.image.width = vih->bmiHeader.biWidth;
							frame.image.height = vih->bmiHeader.biHeight;
							frame.image.format = BitmapFormat::BGR;
							frame.image.pitch =  - frame.image.calculatePitchAlign4(frame.image.width, 24);
							frame.image.data = pBuffer + cbBuffer + frame.image.pitch;
							if (frame.image.format != BitmapFormat::None) {					
								onCaptureVideoFrame(frame);
							}
						}
					}

					return S_OK;
				}

				STDMETHODIMP_(ULONG) AddRef() { return 2; }
				STDMETHODIMP_(ULONG) Release() { return 1; }
				STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
				{
					if (riid == IID_ISampleGrabberCB || riid == IID_IUnknown) {
						*ppv = (void*)(ISampleGrabberCB*)(this);
						return NOERROR;
					}
					return E_NOINTERFACE;
				}

				static List<CameraInfo> _queryDevices(String deviceId, IBaseFilter** filter)
				{
					List<CameraInfo> ret;
					HRESULT hr;

					if (deviceId == "FRONT" || deviceId == "BACK") {
						deviceId.setNull();
					}

					ICreateDevEnum* pDevEnum = NULL;
					hr = CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC, IID_ICreateDevEnum, (void**)&pDevEnum);

					if (pDevEnum) {

						IEnumMoniker* pClassEnum = NULL;
						hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);

						if (pClassEnum) {
							
							IMoniker* pMoniker = NULL;
							ULONG cFetched = 0;

							while (S_OK == pClassEnum->Next(1, &pMoniker, &cFetched)) {

								CameraInfo dev;

								if (pMoniker) {

									IPropertyBag* prop = NULL;
									hr = pMoniker->BindToStorage(0, 0, IID_IPropertyBag, (void**)&prop);

									if (prop) {

										VARIANT var;

										VariantInit(&var);
										hr = prop->Read(L"DevicePath", &var, 0);
										if (SUCCEEDED(hr)) {
											dev.id = String::create(var.bstrVal);
										}
										VariantClear(&var);

										VariantInit(&var);
										hr = prop->Read(L"FriendlyName", &var, 0);
										if (SUCCEEDED(hr)) {
											dev.name = String::create(var.bstrVal);
										}
										VariantClear(&var);

										VariantInit(&var);
										hr = prop->Read(L"Description", &var, 0);
										if (SUCCEEDED(hr)) {
											dev.description = String::create(var.bstrVal);
										}
										VariantClear(&var);

										prop->Release();

									}

									if (dev.id.isNotEmpty()) {
										if (filter) {
											if (deviceId.isEmpty() || deviceId == dev.id) {
												hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)filter);
												if (FAILED(hr)) {
													LOG_HRESULT("Failed to bind Filter", hr);
												}
												return ret;
											}
										} else {
											ret.add_NoLock(dev);
										}
									}
									
									pMoniker->Release();
									pMoniker = NULL;
								}
							}

							pClassEnum->Release();

						} else {
							LOG_HRESULT("Failed to create CLSID_VideoInputDeviceCategory", hr);
						}

						pDevEnum->Release();

					} else {
						LOG_HRESULT("Failed to create CLSID_SystemDeviceEnum", hr);
					}

					return ret;

				}
			};

		}
	}

	Ref<Camera> Camera::create(const CameraParam& param)
	{
		return priv::dshow::CameraImpl::_create(param);
	}

	List<CameraInfo> Camera::getCamerasList()
	{
		return priv::dshow::CameraImpl::_queryDevices(String::null(), NULL);
	}

}

#endif
