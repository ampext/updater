#pragma once

#define CURL_STATICLIB

#include <curl/curl.h>
#include <wx/string.h>
#include <functional>
#include <array>

class UrlHelper
{
public:
	typedef std::function<size_t (const void *, size_t)> WriteFunc;
	typedef std::function<bool (wxLongLong_t, wxLongLong_t)> WriteProgressFunc;

private:
	CURL *curl;
	wxString url;
	wxString initError;

	WriteFunc writeFunc;
	WriteProgressFunc progressFunc;
	bool aborted;

	std::array<char, CURL_ERROR_SIZE> error_buf;

private:
	static size_t WriteDataFunc(char *ptr, size_t size, size_t nmemb, void *userdata);
	static int ProgressFunc(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);

	void LogError(CURLcode code);

public:
	UrlHelper(const wxString &url);
	~UrlHelper();

	wxString GetUrl() const;
	bool IsOk() const;
	wxString GetError() const;

	int GetDownloadSpeed() const;

	void SetWriteDataCallback(const WriteFunc &func);
	void SetProgressCallback(const WriteProgressFunc &func);
	bool Perform();
};
