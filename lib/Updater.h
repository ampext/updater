#pragma once

#include "UpdateInfo.h"
#include "Utils.h"

#include <wx/xml/xml.h>
#include <wx/config.h>

#include <functional>

class Updater
{
public:
	enum UpdateAction
	{
		UpdateDownload,
		UpdateInstall,
		UpdateCheckSum
	};

	typedef std::function<bool (UpdateAction, wxLongLong_t, wxLongLong_t, wxLongLong_t, const wxString&)> ProgressFunc;
	typedef std::function<bool (wxLongLong_t, wxLongLong_t, wxLongLong_t)> DownloadFunc;
	typedef std::function<bool (wxLongLong_t, wxLongLong_t, const wxString&)> ExtractFunc;

private:
	wxString baseURL;
	wxString appName;
	wxString procName;

	ProgressFunc progressFunc;

private:
	wxString GetUpdateInfo(const wxString &name) const;
	
	UpdateInfo ParseUpdateInfoXML(const wxString &src) const;
	std::shared_ptr<UpdateElement> ParseElementNode(wxXmlNode *elementNode) const;
	static wxXmlNode *GetChildren(const wxString &name, wxXmlNode *parent);
	
public:
	Updater(const wxString &url);

	void SetBaseURL(const wxString &url);
	wxString GetBaseURL() const;

	bool Check(UpdateInfo *info = nullptr);
	bool DownloadUpdate(const UpdateInfo &info, wxString &updFileName);
	bool InstallUpdate(const wxString &updFileName, const wxString &dstPath);
	bool UpdateVersionInfo(const UpdateInfo &info);

	void SetProgressCallback(const ProgressFunc &func);

	static bool DownloadFile(const wxString &srcUrl, const wxString &dstPath, const DownloadFunc &callback = DownloadFunc());
	static bool ExtractFilesFromZip(const wxString &srcPath, const wxString &dstPath, const ExtractFunc &callback = ExtractFunc());
	static wxString GetFileSHA1(const wxString &path);
};