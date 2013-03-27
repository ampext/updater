#include "UpdaterApp.h"
#include "UpdateDlg.h"
#include "DownloadDlg.h"

#include <wx/msgdlg.h> 
#include <wx/xrc/xmlres.h>
#include <wx/fileconf.h>
#include <wx/filefn.h> 
#include <wx/stdpaths.h>
#include <wx/dir.h>
#include <wx/socket.h>

#ifdef __WXMSW__
	#include "Shellapi.h"
#endif

#include <exception>

#define wxLOG_COMPONENT "ui"

wxDEFINE_EVENT(wxEVT_RESTART_AND_UPDATE, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_SHOW_NOTIFICATION, wxCommandEvent);

extern void InitXmlResource();

UpdaterApp::UpdaterApp() : trayIcon(nullptr), dlg(nullptr)
{

}

bool UpdaterApp::OnInit()
{
	if(!CheckInstance())
		return false;
	
	SetupLogger();
	SetupConfig();

	if(!wxAppConsole::OnInit())
	{
		wxLogError(L"Failed to init application");
		return false;
	}

	Bind(wxEVT_RESTART_AND_UPDATE, &UpdaterApp::OnRestartAndUpdate, this);
	Bind(wxEVT_SHOW_NOTIFICATION, &UpdaterApp::OnShowNotification, this);

	if(do_test)
	{
		RestartAndUpdate();
		return false;
	}

	if(auto_update)
	{
		wxLogMessage(L"Starting autoupdate");

		DoAutoUpdate();
		ExecuteOriginal(autoupdate_dest);
		OnExit();

		return false;
	}
	else
	{
		wxImage::AddHandler(new wxPNGHandler);

		wxXmlResource::Get()->InitAllHandlers();
		InitXmlResource();

		if(!SetupTray())
		{
			wxLogError(L"Failed to setup application tray");
			return false;
		}

		dlg.reset(new UpdateDlg());
		dlg->Show(!dlg->HideAfterStart());
	}

	return true;
}

void UpdaterApp::OnInitCmdLine(wxCmdLineParser &parser)
{
	static const wxCmdLineEntryDesc desc[] =
	{
		{wxCMD_LINE_OPTION, "a", "autoupdate", "do autoupdate", wxCMD_LINE_VAL_STRING, 0},
		{wxCMD_LINE_SWITCH, "f", "force", "force autoupdate", wxCMD_LINE_VAL_NONE, 0},
		{wxCMD_LINE_OPTION, "p", "postupdate", "autoupdate destination", wxCMD_LINE_VAL_STRING, 0},
		{wxCMD_LINE_PARAM, "", "", "update url", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL},
		//{wxCMD_LINE_SWITCH, "t", "test", "test", wxCMD_LINE_VAL_NONE, 0},
		{wxCMD_LINE_NONE}
	};

    parser.SetDesc(desc);
    parser.SetSwitchChars(L"-");
    parser.EnableLongOptions(true);
}

bool UpdaterApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
	auto_update = parser.Found(L"a", &autoupdate_dest);
	post_update = parser.Found(L"p", &postupdate_dest);
	force_update = parser.FoundSwitch(L"f") == wxCMD_SWITCH_ON;
	do_test = parser.FoundSwitch(L"t") == wxCMD_SWITCH_ON;

	if(auto_update && post_update)
	{
		wxLogError(L"Incompatible command line options '%s' and '%s'", L"--autoupdate", L"--postupdate");
		return false;
	}

	if(auto_update)
	{
		if(parser.GetParamCount() > 0) update_url = parser.GetParam(0);
		else
		{
			wxLogError(L"Update URL not found");
			return false;
		}
	}

	return true;
}

bool UpdaterApp::SetupTray()
{
	if(!wxTaskBarIcon::IsAvailable()) return false;

	auto menuCreator = [this] () -> wxMenu *
	{
		wxMenu *menu = new wxMenu;

		wxMenuItem *checkMenuItem = menu->Append(wxID_ANY, L"Check updates...");
		wxMenuItem *settingsMenuItem = menu->Append(wxID_ANY, L"Settings updates...");

		menu->AppendSeparator();
		wxMenuItem *updMenuItem = menu->Append(wxID_ANY, this->GetUpdateDeltaString());
		menu->Enable(updMenuItem->GetId(), false);
		menu->AppendSeparator();

		wxMenuItem *quitMenuItem = menu->Append(wxID_ANY, L"Quit");

		menu->Bind(wxEVT_COMMAND_MENU_SELECTED, &UpdaterApp::OnCheckUpdates, this, checkMenuItem->GetId());
		menu->Bind(wxEVT_COMMAND_MENU_SELECTED, &UpdaterApp::OnSettings, this, settingsMenuItem->GetId());
		menu->Bind(wxEVT_COMMAND_MENU_SELECTED, &UpdaterApp::OnExit, this, quitMenuItem->GetId());

		return menu;
	};

	trayIcon.reset(new TrayIcon(menuCreator));

	return true;
}

bool UpdaterApp::CheckInstance()
{
	// wxApp is not initialized, so cmd line params are not parsed
	// gettin needed autoupdate param from raw main arguments

	for(int i = 1; i < argc; i++)
	{
		wxString arg = argv[i];
		if(arg == L"-a" || arg == L"--autoupdate") {auto_update = true; break;}
	}

	if(!CheckSingleInstance())
	{
		if(!auto_update) return false;

		// Trying to wait for another copy termination (time limit - 10 seconds)

		bool terminated = false;
		std::cout << "Waiting for another copy termination";

		for(size_t i = 0; i < 10 && !terminated; i++)
		{
			std::cout << ".";
			wxSleep(1);

			terminated = CheckSingleInstance();
		}

		std::cout << std::endl;

		if(!terminated)
		{
			wxLogError("Another copy of program already running. Please close the program before upgrading.");
			std::cout << "Exiting..." << std::endl;
			return false;
		}
	}

	return true;
}

int UpdaterApp::OnExit()
{
	trayIcon.reset();
	dlg.reset();

	BaseApp::OnExit();
	return 0;
}

void UpdaterApp::OnCheckUpdates(wxCommandEvent &event)
{
	if(dlg) dlg->ShowUpdates();
}

void UpdaterApp::OnSettings(wxCommandEvent &event)
{
	if(dlg) dlg->ShowSettings();
}

void UpdaterApp::OnExit(wxCommandEvent &event)
{
	if(wxMessageBox(L"Exit Updater?", L"Confirmation", wxYES_NO, dlg.get()) == wxNO) return;
	Exit();	
}

bool UpdaterApp::CopyToTmpAndExec()
{
	wxString tmpFilePath = wxFileName::GetTempDir() + wxFileName::GetPathSeparator() + L"updater" + wxFileName::GetPathSeparator();
	wxFileName tmpUpdFileName(tmpFilePath + L"upd_ui.exe");
	wxFileName localUpdFileName(wxStandardPaths().GetExecutablePath());

	if(!wxDirExists(tmpFilePath))
	{
		wxLogMessage(L"Creating path '%s'", tmpFilePath);
		wxMkDir(tmpFilePath, wxS_DIR_DEFAULT);
	}

	if(!wxCopyFile(localUpdFileName.GetFullPath(), tmpUpdFileName.GetFullPath(), true))
	{
		wxLogError(L"Can not copy executable to '%s'", tmpUpdFileName.GetFullPath());
		return false;
	}

	bool need_admin_right = !wxFileName::IsDirWritable(localUpdFileName.GetPath());
	wxString cmd_line = wxString::Format(L"--autoupdate=%s %s", localUpdFileName.GetPath(), UpdaterProvider().GetUpdateURL());

	if(!need_admin_right)
	{
		wxLogMessage("Updater directory '%s' is writable", localUpdFileName.GetPath());
	}

	#ifdef __WXMSW__
		HINSTANCE hInst = ShellExecute(NULL, (need_admin_right ? L"runas" : L"open"), tmpUpdFileName.GetFullPath().ToStdWstring().c_str(), cmd_line.ToStdWstring().c_str(), NULL, SW_SHOWNORMAL);
		if((int) hInst <= 32)
		{
			wxLogError("ShellExecute failed with %d code", (int) hInst);
			return false;
		}
	#endif

	return true;
}

bool UpdaterApp::ExecuteOriginal(const wxString &path)
{
	wxFileName updFileName(path +  wxFileName::GetPathSeparator() + L"upd_ui.exe");
	wxString cmd_line = wxString::Format(L"--postupdate=%s", wxStandardPaths().GetExecutablePath());

	wxLogMessage(L"Executing %s with %s", updFileName.GetFullPath(), cmd_line);

	#ifdef __WXMSW__
		HINSTANCE hInst = ShellExecute(NULL, L"open", updFileName.GetFullPath().ToStdWstring().c_str(), cmd_line.ToStdWstring().c_str(), NULL, SW_SHOWNORMAL);
		if((int) hInst <= 32)
		{
			wxLogError("ShellExecute failed with %d code", (int) hInst);
			return false;
		}
	#endif

	return true;
}

bool UpdaterApp::DoAutoUpdate()
{
	if(autoupdate_dest.IsEmpty())
	{
		wxLogError(L"Unknown update destination");
		return false;
	}

	UpdaterProvider updProvider(autoupdate_dest, update_url);

	DownloadDlg dlg(nullptr, wxID_ANY, &updProvider, L"Installing updates");
	int res = dlg.ShowModal();

	if(res == DownloadDlg::UPDATE_OK) wxMessageBox(L"Update(s) successfully installed", L"Information");
	else if(res == DownloadDlg::UPDATE_CANCEL) wxMessageBox(L"Update canceled", L"Information", wxICON_WARNING | wxOK | wxCENTER);
	else if(res == DownloadDlg::UPDATE_FAIL) wxMessageBox(L"Update failed", L"Information", wxICON_ERROR | wxOK | wxCENTER);

	return res == DownloadDlg::UPDATE_OK;
}

void UpdaterApp::OnRestartAndUpdate(wxCommandEvent &event)
{
	RestartAndUpdate();
}

void UpdaterApp::RestartAndUpdate()
{
	if(CopyToTmpAndExec())
	{
		wxLogMessage(L"Updater with --autoupdate has been executed");
	}

	Exit();
}

void UpdaterApp::OnShowNotification(wxCommandEvent &event)
{
	trayIcon->ShowBalloon(L"New updates available", event.GetString(), 5000, wxICON_INFORMATION);
}

wxString UpdaterApp::GetUpdateDeltaString() const
{
	wxTimeSpan delta = dlg->GetTimeToNextUpdate();
	return delta.Format(L"Next check updates in %H:%M:%S");
}

IMPLEMENT_APP(UpdaterApp)
