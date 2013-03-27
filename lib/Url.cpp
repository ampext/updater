#include "Url.h"
#include "Utils.h"

#include <wx/log.h>

#define wxLOG_COMPONENT "updater"

UrlHelper::UrlHelper(const wxString &url) : curl(nullptr), url(url), aborted(false)
{
	curl = curl_easy_init();

	if(!curl)
	{
		wxLogError(L"Can not init curl");
	}

	try
	{
		CURLcode res;

		res = curl_easy_setopt(curl, CURLOPT_URL, url.ToStdString().c_str());
		if(res !=0 ) throw Exception(curl_easy_strerror(res));

		curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, true);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);

		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &WriteDataFunc);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);

		curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, &ProgressFunc);
		curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
		curl_easy_setopt(curl, CURLOPT_NOPROGRESS, false);

		curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buf.data());
	}
	catch(Exception &e)
	{
		initError = e.GetDescription();
	}
}

UrlHelper::~UrlHelper()
{
	if(curl) curl_easy_cleanup(curl);
}

bool UrlHelper::IsOk() const
{
	return curl && initError.IsEmpty();
}

wxString UrlHelper::GetError() const
{
	return initError;
}

wxString UrlHelper::GetUrl() const
{
	return url;
}

void UrlHelper::SetWriteDataCallback(const WriteFunc &func)
{
	writeFunc = func;
}

void UrlHelper::SetProgressCallback(const WriteProgressFunc &func)
{
	progressFunc = func;
}

bool UrlHelper::Perform()
{
	CURLcode res = curl_easy_perform(curl);
	if(res != 0)
	{
		LogError(res);
		return false;
	}

	return true;
}

size_t UrlHelper::WriteDataFunc(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	UrlHelper *url = static_cast<UrlHelper *>(userdata);

	if(url && url->writeFunc) return url->writeFunc(ptr, size * nmemb);
	return 0;
}

int UrlHelper::ProgressFunc(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
	UrlHelper *url = static_cast<UrlHelper *>(clientp);

	if(url && url->aborted) return 1;
	if(url && url->progressFunc && dlnow > 0) return url->aborted = url->progressFunc(static_cast<wxULongLong_t>(dlnow), static_cast<wxULongLong_t>(dltotal));

	return 0;
}

void UrlHelper::LogError(CURLcode code)
{
	wxLogError(L"Curl: %s", curl_easy_strerror(code));
}

int UrlHelper::GetDownloadSpeed() const
{
	double val;

	CURLcode res = curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD , &val);
	if(res != 0) return 0;

	return static_cast<int>(val);
}