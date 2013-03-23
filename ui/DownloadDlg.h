#pragma once

#include <wx/dialog.h>
#include <wx/button.h>

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
	void EndModal(int code);
	void OnUpdateThread(DownloadThreadEvent &event);
	void OnUpdateThreadCompleted(wxThreadEvent &event);

private:
	DownloadPanel *panel;
	wxButton *actionButton;
	DownloadThread *thread;
};
