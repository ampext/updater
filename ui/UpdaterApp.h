#pragma once

#include <memory>

#include <wx/app.h>
#include <wx/cmdline.h>

#include "UpdateDlg.h"
#include "Tray.h"

#define wxLOG_COMPONENT "ui"
#include "BaseApp.h"
#undef wxLOG_COMPONENT

wxDECLARE_EVENT(wxEVT_RESTART_AND_UPDATE, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_SHOW_NOTIFICATION, wxCommandEvent);

class UpdaterApp : public wxApp, public BaseApp
{
public:
	UpdaterApp();
	
private:
	bool SetupTray();

	bool OnInit();
	int OnExit();

	void OnInitCmdLine(wxCmdLineParser &parser);
	bool OnCmdLineParsed(wxCmdLineParser& parser);

	void OnCheckUpdates(wxCommandEvent &event);
	void OnSettings(wxCommandEvent &event);
	void OnExit(wxCommandEvent &event);
	void OnRestartAndUpdate(wxCommandEvent &event);
	void OnShowNotification(wxCommandEvent &event);

	bool CheckInstance();
	bool CopyToTmpAndExec();
	bool ExecuteOriginal(const wxString &path);
	bool DoAutoUpdate();
	void RestartAndUpdate();

	wxString GetUpdateDeltaString() const;
	void LogOwnerInfo() const;

private:
	std::unique_ptr<TrayIcon> trayIcon;
	std::unique_ptr<UpdateDlg> dlg;

	bool auto_update;
	bool post_update;
	bool force_update;
	bool do_test;
	wxString update_url;
	wxString autoupdate_dest;
	wxString postupdate_dest;
};