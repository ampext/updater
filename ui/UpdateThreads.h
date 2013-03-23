#pragma once

#include <wx/thread.h>
#include <wx/event.h>

#include "lib/UpdateInfo.h"
#include "lib/AppInfoProvider.h"

class CheckThreadEvent : public wxThreadEvent
{
private:
	std::vector<UpdateInfo> info;

public:
	CheckThreadEvent(wxEventType eventType = wxEVT_THREAD, int id = wxID_ANY, const std::vector<UpdateInfo> &info = std::vector<UpdateInfo>());
	
	wxEvent *Clone() const;

	const std::vector<UpdateInfo> &GetInfo() const;
	void SetInfo(const std::vector<UpdateInfo> &info);
};

class DownloadThreadEvent : public wxThreadEvent
{
private:
	wxString action;
	wxString statusMsg;
	int progress;

public:
	DownloadThreadEvent(wxEventType eventType = wxEVT_THREAD, int id = wxID_ANY);
	DownloadThreadEvent(const DownloadThreadEvent &event);
	
	wxEvent *Clone() const;

	wxString GetAction() const;
	void SetAction(const wxString &action);

	wxString GetStatusMsg() const;
	void SetStatusMsg(const wxString &msg);

	int GetProgress() const;
	void SetProgress(int value);
};

wxDECLARE_EVENT(wxEVT_COMMAND_CHECKTHREAD_COMPLETED, CheckThreadEvent);
wxDECLARE_EVENT(wxEVT_COMMAND_DOWNLOADTHREAD_COMPLETED, wxThreadEvent);
wxDECLARE_EVENT(wxEVT_COMMAND_DOWNLOADTHREAD_UPDATE, DownloadThreadEvent);

class CheckThread : public wxThread
{
public:
	CheckThread(wxEvtHandler *handler, const std::vector<wxString> &urls);
	~CheckThread();

private:
	ExitCode Entry();

	wxEvtHandler *handler;
	std::vector<wxString> urls;
};

class DownloadThread : public wxThread
{
public:
	DownloadThread(wxThreadKind kind = wxTHREAD_DETACHED, wxEvtHandler *handler = nullptr, AppInfoProvider *appProvider = nullptr);
	~DownloadThread();

private:
	ExitCode Entry();

	wxEvtHandler *handler;
	wxString url;
	wxString installPath;
	wxString appName;
};