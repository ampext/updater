#pragma once

#include <wx/dialog.h>
#include <wx/button.h>
#include <wx/timer.h>

#include "lib/AppInfoProvider.h"
#include "DownloadPanel.h"
#include "UpdateThreads.h"

class DownloadDlg: public wxDialog
{
public:
	static const int UPDATE_OK = 0;
	static const int UPDATE_CANCEL = 1;
	static const int UPDATE_FAIL = 2;

	DownloadDlg(wxWindow *parent = nullptr, wxWindowID id = wxID_ANY, AppInfoProvider *appProvider = nullptr, const wxString &title = L"Downloading..."); 

private:
	void OnClose(wxCloseEvent &event);
	void OnCancel(wxCommandEvent &event);
	void OnKillApp(wxCommandEvent &event);
	void EndModal(int code);
	void OnUpdateThread(DownloadThreadEvent &event);
	void OnUpdateThreadCompleted(wxThreadEvent &event);
	void OnKillTimer(wxTimerEvent &event);

private:
	DownloadPanel *panel;
	wxButton *actionButton;
	wxButton *termButton;
	DownloadThread *thread;

	wxTimer *termTimer;
	wxString procName;
	bool autoTermination;
	time_t timerStartTime;
	size_t timeLimit;
};
