#include "UpdateThreads.h"

#include "lib/Updater.h"
#include "lib/ProcessUtils.h"

#include <wx/filename.h>
#include <wx/log.h>

#define wxLOG_COMPONENT "ui"

wxDEFINE_EVENT(wxEVT_COMMAND_CHECKTHREAD_COMPLETED, CheckThreadEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_DOWNLOADTHREAD_COMPLETED, wxThreadEvent);
wxDEFINE_EVENT(wxEVT_COMMAND_DOWNLOADTHREAD_UPDATE, DownloadThreadEvent);

CheckThread::CheckThread(wxEvtHandler *handler, const std::vector<wxString> &urls) : handler(handler), urls(urls)
{

}

CheckThread::~CheckThread()
{

}

wxThread::ExitCode CheckThread::Entry()
{
	std::vector<UpdateInfo> info;

	for(size_t i = 0; i < urls.size(); i++)
	{
		Updater updater(urls[i]);
		UpdateInfo upd_info;

		if(!updater.Check(&upd_info))
		{
			wxLogMessage("Failed to check URL '" + urls[i] + "'");
			info.push_back(UpdateInfo());
		}
		else info.push_back(upd_info);
	}

	wxQueueEvent(handler, new CheckThreadEvent(wxEVT_COMMAND_CHECKTHREAD_COMPLETED, wxID_ANY, info));
	return 0;
}

DownloadThread::DownloadThread(wxThreadKind kind, wxEvtHandler *handler, AppInfoProvider *appProvider) : wxThread(kind), handler(handler)
{
	url = appProvider->GetUpdateURL();
	installPath = appProvider->GetInstallDir();
	appName = appProvider->GetName();
}


DownloadThread::~DownloadThread()
{
	
}

wxThread::ExitCode DownloadThread::Entry()
{
	wxThreadEvent *completeEvent = new wxThreadEvent(wxEVT_COMMAND_DOWNLOADTHREAD_COMPLETED, wxID_ANY);
	completeEvent->SetInt(0);

	Updater updater(url);
	UpdateInfo info;

	if(!updater.Check(&info))
	{
		wxLogWarning(L"Nothing to update");

		completeEvent->SetInt(1);
		wxQueueEvent(handler, completeEvent);
		return reinterpret_cast<wxThread::ExitCode>(0);
	}

	if(!info.IsOk())
	{
		wxLogWarning(L"Bad update information");

		wxQueueEvent(handler, completeEvent);
		return reinterpret_cast<wxThread::ExitCode>(1);
	}

	if(info.IsEmpty())
	{
		wxLogWarning(L"Nothing to update (update elements not found)");

		wxQueueEvent(handler, completeEvent);
		return reinterpret_cast<wxThread::ExitCode>(1);
	}

	if(info.app_name != appName)
	{
		wxLogWarning(L"Incorrect appliacation name (%s != %s)", info.app_name, appName);

		wxQueueEvent(handler, completeEvent);
		return reinterpret_cast<wxThread::ExitCode>(1);		
	}

	updater.SetProgressCallback([&] (Updater::UpdateAction action, wxLongLong_t val1, wxLongLong_t val2, const wxString &str) -> bool
	{
		DownloadThreadEvent *event = new DownloadThreadEvent(wxEVT_COMMAND_DOWNLOADTHREAD_UPDATE, wxID_ANY);

		if(action == Updater::UpdateDownload)
		{
			event->SetAction(L"Downloading");
			event->SetStatusMsg(wxString::Format(L"Downloaded %s of %s at %s", wxFileName::GetHumanReadableSize(wxULongLong(val1)), wxFileName::GetHumanReadableSize(wxULongLong(val2)), str));
			event->SetProgress(100 * val1 / val2);
		}
		else if(action == Updater::UpdateInstall)
		{
			event->SetAction(L"Installation");
			event->SetStatusMsg(wxString::Format(L"Copying %llu of %llu", val1, val2));
			event->SetProgress(100 * val1 / val2);
		}
		else if(action == Updater::UpdateCheckSum)
		{
			event->SetAction(L"Checking hash sum");
			event->SetStatusMsg(L"Calculating SHA1");
			event->SetProgress(0);
		}

		wxQueueEvent(handler, event);

		return TestDestroy();
	});

	wxString updFileName;

	if(!TestDestroy() && !updater.DownloadUpdate(info, updFileName))
	{
		wxQueueEvent(handler, completeEvent);
		return reinterpret_cast<wxThread::ExitCode>(1);
	}

	while(!TestDestroy() && ProcessUtils::CheckProcess(info.process_name))
	{
		DownloadThreadEvent *event = new DownloadThreadEvent(wxEVT_COMMAND_DOWNLOADTHREAD_UPDATE, wxID_ANY);

		event->SetAction(L"Waiting for process termination");
		event->SetStatusMsg(wxString::Format(L"Process name: %s", info.process_name));
		event->SetProgress(0);

		wxQueueEvent(handler, event);
		wxSleep(1);
	}

	if(!TestDestroy() && !updater.InstallUpdate(updFileName, installPath))
	{
		wxQueueEvent(handler, completeEvent);
		return reinterpret_cast<wxThread::ExitCode>(1);
	}

	if(!TestDestroy() && !updater.UpdateVersionInfo(info))
	{
		wxQueueEvent(handler, completeEvent);
		return reinterpret_cast<wxThread::ExitCode>(1);
	}

	completeEvent->SetInt(!TestDestroy());
	wxQueueEvent(handler, completeEvent);
	
	return 0;
}

CheckThreadEvent::CheckThreadEvent(wxEventType eventType, int id, const std::vector<UpdateInfo> &info) : wxThreadEvent(eventType, id), info(info)
{

}

wxEvent *CheckThreadEvent::Clone() const
{
	return new CheckThreadEvent(*this);
}

const std::vector<UpdateInfo> &CheckThreadEvent::GetInfo() const
{
	return info;
}

void CheckThreadEvent::SetInfo(const std::vector<UpdateInfo> &info)
{
	this->info = info;
}

DownloadThreadEvent::DownloadThreadEvent(wxEventType eventType, int id) : wxThreadEvent(eventType, id)
{

}

DownloadThreadEvent::DownloadThreadEvent(const DownloadThreadEvent &event) : wxThreadEvent(event)
{
	SetAction(event.GetAction().Clone());
	SetStatusMsg(event.GetStatusMsg().Clone());
	SetProgress(event.GetProgress());
}

wxEvent *DownloadThreadEvent::Clone() const
{
	return new DownloadThreadEvent(*this);
}

wxString DownloadThreadEvent::GetAction() const
{
	return action;
}

void DownloadThreadEvent::SetAction(const wxString &action)
{
	this->action = action;
}

wxString DownloadThreadEvent::GetStatusMsg() const
{
	return statusMsg;
}

void DownloadThreadEvent::SetStatusMsg(const wxString &msg)
{
	statusMsg = msg;
}

int DownloadThreadEvent::GetProgress() const
{
	return progress;
}

void DownloadThreadEvent::SetProgress(int value)
{
	progress = value;
}