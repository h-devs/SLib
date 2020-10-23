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

#include "slib/core/definition.h"

#if defined(SLIB_PLATFORM_IS_WIN32)

#include "slib/network/url_request.h"

#include "slib/core/platform_windows.h"

#include "slib/network/url.h"
#include "slib/core/string_buffer.h"
#include "slib/core/scoped.h"
#include "slib/core/safe_static.h"
#include "slib/core/async.h"
#include "slib/core/system.h"

#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")

#define MAX_CONNECTION_POOL_SIZE 32
#define READ_BUFFER_SIZE 65536
#define DOWNLOAD_ACCUME_SIZE 2000000

#undef min
#undef max

namespace slib
{

	namespace priv
	{
		namespace url_request
		{

			class Connection : public Referable
			{
			public:
				HINTERNET hConnect;
				sl_int32 id;
				String16 address;
				sl_bool flagHttps;

			public:
				Connection(HINTERNET _hConnect, sl_int32 _id, const String16& _address, sl_bool _flagHttps) : hConnect(_hConnect), id(_id), address(_address), flagHttps(_flagHttps)
				{
				}

				~Connection();

			};

			class UrlRequestImpl;

			class Session: public Referable
			{
			public:
				HINTERNET hInternet;
				LinkedQueue< Ref<Connection> > connectionPool;
				sl_int32 lastConnectionId;
				HashMap< sl_int32, WeakRef<Connection> > connections;
				sl_int32 lastTaskId;
				HashMap< sl_int32, WeakRef<UrlRequestImpl> > requests;

			public:
				Session(WINHTTP_STATUS_CALLBACK callback)
				{
					lastConnectionId = 0;
					lastTaskId = 0;
					
					hInternet = WinHttpOpen(L"Windows Client", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, WINHTTP_FLAG_ASYNC);
					WinHttpSetStatusCallback(hInternet, callback,
						WINHTTP_CALLBACK_STATUS_REQUEST_ERROR |
						WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE |
						WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE |
						WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE |
						WINHTTP_CALLBACK_STATUS_READ_COMPLETE,
						NULL);
				}

				~Session()
				{
					WinHttpSetStatusCallback(hInternet, NULL, 0, NULL);
					WinHttpCloseHandle(hInternet);
				}

				Ref<Connection> getConnection(const String& url, String16& outPath)
				{
					String16 urlBuffer = String16::from(url);
					DWORD lenURL = (DWORD)(urlBuffer.getLength());
					URL_COMPONENTSW comps = { 0 };
					comps.dwStructSize = sizeof(comps);
					comps.dwHostNameLength = lenURL;
					comps.dwUserNameLength = lenURL;
					comps.dwPasswordLength = lenURL;
					comps.dwUrlPathLength = lenURL;
					if (WinHttpCrackUrl((LPCWSTR)(urlBuffer.getData()), lenURL, 0, &comps)) {
						if (comps.lpszHostName && (comps.nScheme == INTERNET_SCHEME_HTTP || comps.nScheme == INTERNET_SCHEME_HTTPS)) {
							if (comps.lpszUrlPath && *(comps.lpszUrlPath)) {
								outPath = String16::create(comps.lpszUrlPath);
								*(comps.lpszUrlPath) = 0;
							} else {
								SLIB_STATIC_STRING16(s, "/")
								outPath = s;
							}
							String16 address = urlBuffer.getData();
							{
								ObjectLocker lock(&connectionPool);
								Link< Ref<Connection> >* link = connectionPool.getBack();
								while (link) {
									Ref<Connection>& r = link->value;
									if (r->address.getHashCode() == address.getHashCode() && r->address == address) {
										Ref<Connection> connection = link->value;
										Link< Ref<Connection> >* before = link->before;
										connectionPool.removeAt(link);
										return connection;
									}
									link = link->before;
								}
							}
							comps.lpszHostName[comps.dwHostNameLength] = 0;
							if (comps.lpszUserName) {
								comps.lpszUserName[comps.dwUserNameLength] = 0;
							}
							if (comps.lpszPassword) {
								comps.lpszPassword[comps.dwPasswordLength] = 0;
							}
							sl_int32 connectionId = Base::interlockedIncrement32(&(lastConnectionId)) & 0x7FFFFFFF;
							HINTERNET hConnect = WinHttpConnect(hInternet, comps.lpszHostName, comps.nPort, 0);					
							if (hConnect) {
								Ref<Connection> ret = new Connection(hConnect, connectionId, address, comps.nScheme == INTERNET_SCHEME_HTTPS);
								if (ret.isNotNull()) {
									if (comps.lpszUserName || comps.lpszPassword) {
										WinHttpSetCredentials(hConnect, WINHTTP_AUTH_TARGET_SERVER, WINHTTP_AUTH_SCHEME_BASIC, comps.lpszUserName, comps.lpszPassword, NULL);
									}
									connections.put(connectionId, ret);
									return ret;
								}
								WinHttpCloseHandle(hConnect);
							}
						}
					}
					return sl_null;
				}

			};

			class UrlRequestImpl : public UrlRequest
			{
			public:
				Ref<Connection> m_connection;
				HINTERNET m_hRequest;
				sl_int32 m_taskId;
				sl_int32 m_step;

				Memory m_memReceiving;
				sl_size m_offsetReceiving;

				AtomicRef<AsyncStream> m_fileDownload;
				sl_bool m_flagDownloadReading;
				sl_reg m_sizeDownloadWriting;

				enum {
					STEP_INIT, STEP_SENDING_REQUEST, STEP_RECEIVING_RESPONSE, STEP_RECEIVING_DATA, STEP_FINISHED_RECEIVING, STEP_COMPLETE, STEP_ERROR
				};

			public:
				UrlRequestImpl()
				{
					m_hRequest = NULL;
					m_step = STEP_INIT;

					m_offsetReceiving = 0;

					m_flagDownloadReading = sl_false;
					m_sizeDownloadWriting = 0;
				}

				~UrlRequestImpl()
				{
					clean();
				}

			public:
				static Ref<Session> getSession()
				{
					SLIB_SAFE_LOCAL_STATIC(AtomicRef<Session>, session, new Session(callbackStatus))
					if (SLIB_SAFE_STATIC_CHECK_FREED(session)) {
						return sl_null;
					}
					return session;
				}

				static Ref<UrlRequestImpl> create(const UrlRequestParam& param, const String& url)
				{
					Ref<Session> session = getSession();
					if (session.isNotNull()) {
						String16 path;
						Ref<Connection> connection = session->getConnection(url, path);
						if (connection.isNotNull()) {
							Ref<UrlRequestImpl> ret = new UrlRequestImpl;
							if (ret.isNotNull()) {
								Ref<AsyncStream> fileDownload;
								if (param.downloadFilePath.isNotEmpty()) {
									fileDownload = AsyncFile::openIOCP(param.downloadFilePath, FileMode::Write);
									if (fileDownload.isNull()) {
										return sl_null;
									}
								}
								sl_int32 taskId = Base::interlockedIncrement32(&(session->lastTaskId)) & 0x7FFFFFFF;
								String16 verb = String16::from(HttpMethodHelper::toString(param.method));
								DWORD flags = WINHTTP_FLAG_REFRESH;
								if (connection->flagHttps) {
									flags |= WINHTTP_FLAG_SECURE;
								}
								ret->_init(param, url);
								ret->m_connection = connection;
								ret->m_taskId = taskId;
								ret->m_fileDownload = fileDownload;
								session->requests.put(taskId, ret);
								HINTERNET hRequest = WinHttpOpenRequest(connection->hConnect, (LPCWSTR)(verb.getData()), (LPCWSTR)(path.getData()), NULL, NULL, NULL, flags);
								if (hRequest) {
									WinHttpSetTimeouts(hRequest, param.timeout, param.timeout, param.timeout, param.timeout);
									if (connection->flagHttps && param.flagAllowInsecureConnection) {
										DWORD dwFlags =
											SECURITY_FLAG_IGNORE_UNKNOWN_CA |
											SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE |
											SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
											SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
										WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
									}
									ret->m_hRequest = hRequest;
									return ret;
								}
								session->requests.remove(taskId);
							}
						}
					}
					return sl_null;
				}

				void _cancel() override
				{
					clean();
				}

				void _sendAsync() override
				{
					if (m_hRequest) {
						startAsync();
					}
				}

				void clean()
				{
					if (m_hRequest) {
						WinHttpCloseHandle(m_hRequest);
						m_hRequest = NULL;
						Ref<Session> session = getSession();
						if (session.isNotNull()) {
							session->requests.remove(m_taskId);
						}
						m_connection.setNull();
					}
					m_fileDownload.setNull();
				}

				void startAsync()
				{
					if (m_step != STEP_INIT) {
						return;
					}
					{
						for (auto& pair : m_requestHeaders) {
							String16 line = String16::format("%s: %s\r\n", pair.key, pair.value);
							WinHttpAddRequestHeaders(m_hRequest, (LPCWSTR)(line.getData()), (DWORD)(line.getLength()), WINHTTP_ADDREQ_FLAG_ADD);
						}
					}

					Memory body = m_requestBody;

					m_step = STEP_SENDING_REQUEST;
					if (!(WinHttpSendRequest(m_hRequest, NULL, 0, body.getData(), (DWORD)(body.getSize()), (DWORD)(body.getSize()), (DWORD_PTR)m_taskId))) {
						processLastError();
					}
				}

				void _onSendRequestComplete()
				{
					m_sizeBodySent = m_requestBody.getSize();
					onUploadBody(m_sizeBodySent);

					m_step = STEP_RECEIVING_RESPONSE;
					if (!(WinHttpReceiveResponse(m_hRequest, NULL))) {
						processLastError();
					}
				}

				void _onHeadersAvailable()
				{
					DWORD status = 0;
					DWORD len = 4;
					if (!(WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, NULL, &status, &len, NULL))) {
						processLastError();
						return;
					}
					m_responseStatus = (HttpStatus)status;

					m_sizeContentTotal = SLIB_UINT64_MAX;

					len = 0;
					WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, NULL, &len, NULL);
					SLIB_SCOPED_BUFFER(char, 1024, bufHeaders, len);
					if (!(WinHttpQueryHeaders(m_hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF, NULL, bufHeaders, &len, NULL))) {
						processLastError();
						return;
					}
					if (len) {
						HttpResponse response;
						String strPacket = String::create((sl_char16*)bufHeaders, len / 2);
						sl_reg iRet = response.parseResponsePacket(strPacket.getData(), strPacket.getLength());
						if (iRet > 0) {
							m_responseMessage = response.getResponseMessage();
							m_responseHeaders = response.getResponseHeaders();
							String strLength = response.getResponseHeader("Content-Length");
							if (strLength.isNotEmpty()) {
								m_sizeContentTotal = strLength.parseUint64();
							}
						}
					}

					onResponse();

					if (m_sizeContentTotal == 0) {
						processComplete();
						return;
					}

					m_step = STEP_RECEIVING_DATA;

					if (m_fileDownload.isNotNull()) {
						downloadData();
					} else {
						receiveData();
					}
				}

				void receiveData()
				{
					if (m_step != STEP_RECEIVING_DATA) {
						return;
					}
					if (!(WinHttpQueryDataAvailable(m_hRequest, NULL))) {
						processLastError();
					}
				}

				void downloadData()
				{
					if (m_step != STEP_RECEIVING_DATA) {
						return;
					}
					ObjectLocker lock(this);
					if (m_flagDownloadReading) {
						return;
					}
					if (m_sizeDownloadWriting > DOWNLOAD_ACCUME_SIZE) {
						return;
					}
					m_flagDownloadReading = sl_true;
					lock.unlock();
					if (!(WinHttpQueryDataAvailable(m_hRequest, NULL))) {
						processLastError();
					}
				}

				void _onDataAvailable(sl_size size)
				{
					if (m_step != STEP_RECEIVING_DATA) {
						return;
					}
					if (size == 0) {
						finishReceiving();
						return;
					}
					sl_size sizeBuf = m_memReceiving.getSize();
					if (sizeBuf < size) {
						if (sizeBuf == 0) {
							sizeBuf = Math::max((sl_size)READ_BUFFER_SIZE, size);
							if ((sl_uint64)sizeBuf > m_sizeContentTotal) {
								sizeBuf = (sl_size)m_sizeContentTotal;
							}
						} else {
							processReadData();
							sizeBuf = size;
						}
						m_memReceiving = Memory::create(sizeBuf);
						if (m_memReceiving.isNull()) {
							processError("Memory Error");
							return;
						}
						m_offsetReceiving = 0;
					}
					if (m_offsetReceiving + size > sizeBuf) {
						processReadData();
					}
					if (!(WinHttpReadData(m_hRequest, (char*)(m_memReceiving.getData()) + m_offsetReceiving, (DWORD)size, NULL))) {
						processLastError();
					}
				}

				void finishReceiving()
				{
					processReadData();
					if (m_fileDownload.isNotNull()) {
						Ref<AsyncStream> file = m_fileDownload;
						if (file.isNotNull()) {
							m_step = STEP_FINISHED_RECEIVING;
							if (!(file->write(sl_null, 0, SLIB_FUNCTION_WEAKREF(UrlRequestImpl, _onWriteDownloadFile, this)))) {
								processError("Error on writing download file");
							}
						}
					} else {
						processComplete();
					}
				}

				void processReadData()
				{
					if (m_offsetReceiving == 0) {
						return;
					}
					if (m_fileDownload.isNotNull()) {
						Ref<AsyncStream> file = m_fileDownload;
						if (file.isNotNull()) {
							Memory mem = Memory::create(m_memReceiving.getData(), (sl_uint32)m_offsetReceiving);
							if (mem.isNull()) {
								processError("Error on writing download file");
							} else {
								Base::interlockedAdd(&m_sizeDownloadWriting, m_offsetReceiving);
								if (!(file->writeFromMemory(mem, SLIB_FUNCTION_WEAKREF(UrlRequestImpl, _onWriteDownloadFile, this)))) {
									processError("Error on writing download file");
								}
							}
						}
					} else {
						onReceiveContent(m_memReceiving.getData(), m_offsetReceiving, Memory::null());
					}
					m_offsetReceiving = 0;
				}

				void _onReadComplete(const void* data, DWORD size)
				{
					if (m_step != STEP_RECEIVING_DATA) {
						return;
					}
					m_offsetReceiving += size;
					if (m_sizeContentReceived + size >= m_sizeContentTotal) {				
						finishReceiving();
						return;
					}
					if (m_downloadFilePath.isNotEmpty()) {
						m_flagDownloadReading = sl_false;
						downloadData();
					} else {
						receiveData();
					}
				}

				void _onWriteDownloadFile(AsyncStreamResult& result)
				{
					if (result.size > 0) {
						onDownloadContent(result.size);
						Base::interlockedAdd(&m_sizeDownloadWriting, -((sl_reg)(result.size)));
					}
					if (result.flagError) {
						processError("Error on writing download file");
						return;
					}
					if (m_sizeContentReceived >= m_sizeContentTotal || (m_step == STEP_FINISHED_RECEIVING && result.size == 0)) {
						processComplete();
					} else {
						downloadData();
					}
				}

				static void CALLBACK callbackStatus(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
				{
					if (dwContext == 0) {
						return;
					}
					Ref<Session> session = getSession();
					if (session.isNull()) {
						return;
					}
					Ref<UrlRequestImpl> request = session->requests.getValue((sl_int32)(dwContext), WeakRef<UrlRequestImpl>::null());
					if (request.isNull()) {
						return;
					}
					switch (dwInternetStatus) {
					case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
						request->processErrorCode(((WINHTTP_ASYNC_RESULT*)lpvStatusInformation)->dwError);
						break;
					case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
						request->_onSendRequestComplete();
						break;
					case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
						request->_onHeadersAvailable();
						break;
					case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
						request->_onDataAvailable(*((DWORD*)lpvStatusInformation));
						break;
					case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
						request->_onReadComplete(lpvStatusInformation, dwStatusInformationLength);
						break;
					}
				}

				void processComplete()
				{
					ObjectLocker lock(this);
					if (m_step == STEP_COMPLETE || m_step == STEP_ERROR) {
						return;
					}
					m_step = STEP_COMPLETE;
					onComplete();

					if (m_sizeContentTotal == m_sizeContentReceived || m_responseHeaders.getValue("Transfer-Encoding").compareIgnoreCase("chunked") == 0) {
						Ref<Session> session = getSession();
						if (session.isNotNull()) {
							if (session->connectionPool.getCount() > MAX_CONNECTION_POOL_SIZE) {
								session->connectionPool.pop();
							}
							session->connectionPool.push(m_connection);
						}
					}
					clean();
				}

				void processLastError()
				{
					processErrorCode(GetLastError());
				}

				void processErrorCode(DWORD dwError)
				{
					processError(System::formatErrorCode(dwError));
				}

				void processError(const String& errorMessage)
				{
					ObjectLocker lock(this);
					if (m_step == STEP_COMPLETE || m_step == STEP_ERROR) {
						return;
					}
					m_step = STEP_ERROR;
					m_errorMessage = errorMessage;
					onError();
					clean();
				}

			};

			Connection::~Connection()
			{
				WinHttpCloseHandle(hConnect);
				Ref<Session> session = UrlRequestImpl::getSession();
				if (session.isNotNull()) {
					session->connections.remove(id);
				}
			}

		}
	}

	Ref<UrlRequest> UrlRequest::_create(const UrlRequestParam& param, const String& url)
	{
		return Ref<UrlRequest>::from(priv::url_request::UrlRequestImpl::create(param, url));
	}

}

#endif
