#include "DownloadDlg.h"

#include <wx/sizer.h>
#include <wx/log.h>
#include <wx/fileconf.h>

#include "lib/ProcessUtils.h"
#include "lib/Utils.h"

#define wxLOG_COMPONENT "ui"

DownloadDlg::DownloadDlg(wxWindow *parent, wxWindowID id, const AppInfoProvider *appProvider, const wxString &title) : wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	termTimer = nullptr;

	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

	sizer->Add(panel = new DownloadPanel(this), 1, wxALL | wxEXPAND, 5);

	wxBoxSizer *btnSizer = new wxBoxSizer(wxHORIZONTAL);
	{
		btnSizer->AddStretchSpacer(1);
		btnSizer->Add(termButton = new wxButton(this, wxID_ANY, L"Terminate"), 0, wxALL | wxEXPAND, 0);
		btnSizer->AddSpacer(5);
		btnSizer->Add(actionButton = new wxButton(this, wxID_ANY, L"Cancel"), 0, wxALL | wxEXPAND, 0);
	}
	sizer->Add(btnSizer, 0, wxALL | wxEXPAND, 5);

	SetSizerAndFit(sizer);
	SetSize(wxSize(350, -1));

	panel->SetInfo(L"Preparing", L"Fetching update information...", 0);
	termButton->Hide();

	wxConfigBase *cfg = wxConfigBase::Get();
	{
		autoTermination = ReadConfigValue(cfg, L"/AutoTerminateApp", true);
		timeLimit = ReadConfigValue(cfg, L"/AutoTerminateAppInterval", 30);
	}

	Bind(wxEVT_COMMAND_DOWNLOADTHREAD_COMPLETED, &DownloadDlg::OnUpdateThreadCompleted, this);
	Bind(wxEVT_COMMAND_DOWNLOADTHREAD_UPDATE, &DownloadDlg::OnUpdateThread, this);
	Bind(wxEVT_CLOSE_WINDOW, &DownloadDlg::OnClose, this);

	actionButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DownloadDlg::OnCancel, this);
	termButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DownloadDlg::OnKillApp, this);

	thread = new DownloadThread(wxTHREAD_DETACHED, this, appProvider);

	if(thread->Create() != wxTHREAD_NO_ERROR || thread->Run() != wxTHREAD_NO_ERROR)
	{
		delete thread;
		thread = nullptr;
		
		wxLogError(L"Can not create update thread");
	}
}

void DownloadDlg::OnClose(wxCloseEvent &event)
{
	wxCommandEvent evt;
	OnCancel(evt);
}

void DownloadDlg::EndModal(int code)
{
	if(termTimer) termTimer->Stop();
	wxDialog::EndModal(code);
}

void DownloadDlg::OnCancel(wxCommandEvent &event)
{
	if(thread)
	{
		wxThread *tmp_thread = thread;
		thread = nullptr; // little trick

		tmp_thread->Delete();

		wxLogMessage(L"Canceling update...");
	}
	else EndModal(UPDATE_CANCEL);
}

void DownloadDlg::OnKillApp(wxCommandEvent &event)
{
	if(procName.IsEmpty())
	{
		wxLogWarning(L"Can not kill process with empty name");
	}
	else
	{
		if(!ProcessUtils::KillProcess(procName))
			termButton->Disable();
	}
}

void DownloadDlg::OnUpdateThread(DownloadThreadEvent& event)
{
	panel->SetInfo(event.GetAction(), event.GetStatusMsg(), event.GetProgress());
	termButton->Show(event.GetTerminateFlag());

	if(event.GetTerminateFlag()) procName = event.GetString();

	if(autoTermination && event.GetTerminateFlag() && !termTimer)
	{
		termTimer = new wxTimer(this);
		Connect(termTimer->GetId(), wxEVT_TIMER, wxTimerEventHandler(DownloadDlg::OnKillTimer));
		termTimer->Start(1000);

		timerStartTime = time(nullptr);

		termButton->Show();
		termButton->SetLabel(wxString::Format(L"Terminate (%d)", timeLimit));
	}
}

void DownloadDlg::OnUpdateThreadCompleted(wxThreadEvent &event)
{
	EndModal(thread == nullptr ? UPDATE_CANCEL : (event.GetInt() ? UPDATE_OK : UPDATE_FAIL));
}

void DownloadDlg::OnKillTimer(wxTimerEvent &event)
{
	int time_left = time(nullptr) - timerStartTime;

	termButton->SetLabel(wxString::Format(L"Terminate (%d)", std::max<int>(0, timeLimit - time_left)));

	if((timeLimit - time_left) <= 0)
	{
		termTimer->Stop();
		wxQueueEvent(termButton, new wxCommandEvent(wxEVT_COMMAND_BUTTON_CLICKED, termButton->GetId()));
	}
}