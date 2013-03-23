#include "DownloadDlg.h"

#include <wx/sizer.h>
#include <wx/log.h>

#define wxLOG_COMPONENT "ui"

DownloadDlg::DownloadDlg(wxWindow *parent, wxWindowID id, AppInfoProvider *appProvider, const wxString &title) : wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

	sizer->Add(panel = new DownloadPanel(this), 1, wxALL | wxEXPAND, 5);
	sizer->Add(actionButton = new wxButton(this, wxID_ANY, "Cancel"), 0, wxALL | wxALIGN_RIGHT, 5); 

	SetSizerAndFit(sizer);
	SetSize(wxSize(300, -1));

	panel->SetInfo(L"Preparing", L"Fetching update information...", 0);

	Bind(wxEVT_COMMAND_DOWNLOADTHREAD_COMPLETED, &DownloadDlg::OnUpdateThreadCompleted, this);
	Bind(wxEVT_COMMAND_DOWNLOADTHREAD_UPDATE, &DownloadDlg::OnUpdateThread, this);
	Bind(wxEVT_CLOSE_WINDOW, &DownloadDlg::OnClose, this);
	actionButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &DownloadDlg::OnCancel, this);

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

void DownloadDlg::OnUpdateThread(DownloadThreadEvent& event)
{
	panel->SetInfo(event.GetAction(), event.GetStatusMsg(), event.GetProgress());
}

void DownloadDlg::OnUpdateThreadCompleted(wxThreadEvent &event)
{
	EndModal(thread == nullptr ? UPDATE_CANCEL : (event.GetInt() ? UPDATE_OK : UPDATE_FAIL));
}