#include "slib/storage/restfs.h"

namespace slib
{

	RestFs::RestFs(String Url) : m_baseUrl(Url) {
		m_volumeInfo.volumeName = "RestFs";
		m_volumeInfo.fileSystemName = "RestFs";
		m_volumeInfo.totalSize = 1024 * 1024 * 1024;
		m_volumeInfo.freeSize = 512 * 1024 * 1024;

		if (!ping())
			throw FileSystemError::InitFailure;
	}

	sl_bool RestFs::ping()
	{
		String url = String::format("%s?cmd=ping", m_baseUrl);
		auto req = UrlRequest::sendSynchronous(Url::encodeUri(url));
		if (req->getResponseStatus() != HttpStatus::OK)
			return sl_false;
		String res = req->getResponseContentAsString();
		if (res.isEmpty())
			return sl_false;
		return sl_true;
	}

	String parseEntry(String entry, FileInfo &info)
	{
		List<String> list = entry.trim().split("|");
		if (list.getCount() < 4)
			return sl_null;
		String type = list[0];
		String name = list[1];
		String date = list[2];
		String size = list[3];
		info.attr.isDirectory = type == "DIR";
		info.createdAt = info.modifiedAt = info.lastAccessedAt = Time::fromUnixTime(date.parseInt64());
		info.size = info.allocationSize = size.parseUint64();
		return name;
	}

	FileInfo RestFs::afsGetFileInfo(String uri)
	{
		String url = String::format("%s%s?cmd=info", m_baseUrl, uri.replaceAll("\\", "/"));
		auto req = UrlRequest::sendSynchronous(Url::encodeUri(url));
		if (req->getResponseStatus() != HttpStatus::OK)
			throw getErrorFromResponse(req);

		String res = req->getResponseContentAsString();
		if (res.isEmpty())
			throw FileSystemError::NotFound;

		FileInfo info;
		if (parseEntry(res, info).isEmpty())
			throw FileSystemError::GeneralError;
		return info;
	}

	HashMap<String, FileInfo> RestFs::afsFindFiles(String uri) 
	{
		String url = String::format("%s%s?cmd=list", m_baseUrl, uri.replaceAll("\\", "/"));
		auto req = UrlRequest::sendSynchronous(Url::encodeUri(url));
		if (req->getResponseStatus() != HttpStatus::OK)
			throw getErrorFromResponse(req);

		HashMap<String, FileInfo> files;
		auto body = req->getResponseContentAsString();
		for (auto& line : body.split("\n")) {
			line = line.trim();
			if (line.isEmpty()) break;

			FileInfo info;
			String name = parseEntry(line, info);
			if (name.isEmpty()) continue;	// WARNING
			files.add(name, info);
		}
		return files;
	}

	sl_size RestFs::afsRead(String uri, const Memory& buffer, sl_uint64 offset)
	{
		String url = String::format("%s%s?cmd=read&offset=%d&len=%d",
			m_baseUrl, uri.replaceAll("\\", "/"), offset, buffer.getSize());
		auto req = UrlRequest::sendSynchronous(Url::encodeUri(url));
		if (req->getResponseStatus() != HttpStatus::OK)
			throw getErrorFromResponse(req);

		return buffer.copy(req->getResponseContent());
	}

	sl_size RestFs::afsWrite(String uri, const Memory& buffer, sl_uint64 offset)
	{
		String url = String::format("%s%s?cmd=write&offset=%d&len=%d",
			m_baseUrl, uri.replaceAll("\\", "/"), offset, buffer.getSize());
		auto req = UrlRequest::sendSynchronous(HttpMethod::PUT, Url::encodeUri(url), buffer);
		if (req->getResponseStatus() != HttpStatus::OK)
			throw getErrorFromResponse(req);
		String res = req->getResponseContentAsString();
		if (res.isEmpty())
			throw FileSystemError::AccessDenied;

		return res.parseInt32();
	}

	FileInfo RestFs::afsCreateNew(String uri, sl_bool isDirectory)
	{
		String url = String::format("%s%s?cmd=create&isdir=%s",
			m_baseUrl, uri.replaceAll("\\", "/"), isDirectory ? "1" : "");
		auto req = UrlRequest::sendSynchronous(HttpMethod::POST, Url::encodeUri(url));
		if (req->getResponseStatus() != HttpStatus::OK)
			throw getErrorFromResponse(req);

		String res = req->getResponseContentAsString();
		if (res.isEmpty())
			throw FileSystemError::NotFound;

		FileInfo info;
		if (parseEntry(res, info).isEmpty())
			throw FileSystemError::GeneralError;
		return info;
	}

	void RestFs::afsSetFileSize(String uri, sl_uint64 size) 
	{
		String url = String::format("%s%s?cmd=set&size=%d", 
			m_baseUrl, uri.replaceAll("\\", "/"), size);
		auto req = UrlRequest::sendSynchronous(HttpMethod::POST, Url::encodeUri(url));
		if (req->getResponseStatus() != HttpStatus::OK)
			throw getErrorFromResponse(req);

		String res = req->getResponseContentAsString();
		if (res.isEmpty())
			throw FileSystemError::NotFound;
	}

	void RestFs::afsRename(String uri, String newUri, sl_bool replaceIfExists)
	{
		String url = String::format("%s%s?cmd=rename&to=%s%s&replace=%s",
			m_baseUrl, uri.replaceAll("\\", "/"), 
			Url(m_baseUrl).path, newUri.replaceAll("\\", "/"),
			replaceIfExists ? "1" : "");
		auto req = UrlRequest::sendSynchronous(HttpMethod::POST, Url::encodeUri(url));
		if (req->getResponseStatus() != HttpStatus::OK)
			throw getErrorFromResponse(req);

		String res = req->getResponseContentAsString();
		if (res.isEmpty())
			throw FileSystemError::NotFound;
	}

	void RestFs::afsDelete(String uri, sl_bool checkOnly) 
	{
		String url = String::format("%s%s?checkonly=%s",
			m_baseUrl, uri.replaceAll("\\", "/"), checkOnly ? "1" : "");
		auto req = UrlRequest::sendSynchronous(HttpMethod::DELETE, Url::encodeUri(url));
		if (req->getResponseStatus() != HttpStatus::OK)
			throw getErrorFromResponse(req);

		String res = req->getResponseContentAsString();
		if (res.isEmpty())
			throw FileSystemError::GeneralError;
	}

	FileSystemError RestFs::getErrorFromResponse(Ref<UrlRequest> req)
	{
		HttpStatus status = req->getResponseStatus();
		if (status == HttpStatus::NotImplemented)
			return FileSystemError::NotImplemented;
		if (status == HttpStatus::NotFound)
			return FileSystemError::NotFound;
		if ((int)status >= 400)
			return FileSystemError::AccessDenied;
		return FileSystemError::GeneralError;
	}

}