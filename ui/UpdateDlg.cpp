#include "UpdateDlg.h"
#include "WaitDlg.h"
#include "DownloadDlg.h"
#include "DataViewBitmapRenderer.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/timectrl.h>
#include <wx/xrc/xmlres.h>

#include "lib/Updater.h"
#include "UpdaterApp.h"

#define wxLOG_COMPONENT "ui"

const long WAIT_DLG = wxNewId();
const long DOWNLOAD_DLG = wxNewId();

UpdateDlg::UpdateDlg(wxWindow *parent) : wxDialog(parent, wxID_ANY, L"Updates", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
{
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

	nbWidget = new wxNotebook(this, wxID_ANY);

	nbWidget->AddPage(CreateUpdatesPage(nbWidget), L"Updates");
	nbWidget->AddPage(CreateSettingsPage(nbWidget), L"Settings");

	sizer->Add(nbWidget, 1, wxALL | wxEXPAND, 5);
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL | wxAPPLY), 0, wxALL | wxEXPAND, 5); 

	Bind(wxEVT_COMMAND_CHECKTHREAD_COMPLETED, &UpdateDlg::OnCheckThreadCompleted, this);
	Bind(wxEVT_CLOSE_WINDOW, &UpdateDlg::OnClose, this);

	Connect(wxID_OK, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(UpdateDlg::OnOK));
	Connect(wxID_APPLY, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(UpdateDlg::OnApply));
	Connect(wxID_CANCEL, wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(UpdateDlg::OnCancel));

	EnableApplyButton(false);
	SetupTimer();

	SetSizerAndFit(sizer);
	SetSize(wxSize(480, 300));

	LoadSettings();

	if(startCheck->GetValue()) CheckUpdates();
}

wxWindow *UpdateDlg::CreateUpdatesPage(wxWindow *parent)
{
	wxPanel *panel = new wxPanel(parent);
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

	updListCtrl = new wxDataViewListCtrl(panel, wxID_ANY);

	wxDataViewColumn *statusColumn = new wxDataViewColumn(wxEmptyString, new DataViewBitmapRenderer(), 0, 24, wxALIGN_CENTER, 0);
    updListCtrl->AppendColumn(statusColumn);

	updListCtrl->AppendTextColumn(L"Name");
	updListCtrl->AppendTextColumn(L"Installed");
	updListCtrl->AppendTextColumn(L"Version");
	updListCtrl->AppendTextColumn(L"Size");

	columnsIds = {L"StatusColumn", L"NameColumn", L"LocalVersionColumn", L"UpdateVersionColumn", L"SizeColumn"};

	sizer->Add(updListCtrl, 1, wxALL | wxEXPAND, 5);

	wxBoxSizer *subSizer = new wxBoxSizer(wxHORIZONTAL);
	{
		subSizer->Add(lastUpdLabel = new wxStaticText(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxST_ELLIPSIZE_END | wxST_NO_AUTORESIZE | wxALIGN_LEFT), 1, wxALL | wxALIGN_CENTER_VERTICAL, 5);
		subSizer->Add(updButton = new wxButton(panel, wxID_ANY, L"Check for updates..."), 0, wxALL | wxEXPAND, 5);
		subSizer->Add(installButton = new wxButton(panel, wxID_ANY, L"Install"), 0, wxALL | wxEXPAND, 5);

		installButton->Enable(false);

		updButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &UpdateDlg::OnCheckUpdates, this);
		installButton->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &UpdateDlg::OnInstallUpdates, this);
	}
	sizer->Add(subSizer, 0, wxALL | wxEXPAND, 0);

	wxString strDate;
	wxConfigBase::Get()->Read(L"/LastChecked", &strDate);

	lastUpdLabel->SetLabel(wxString::Format(L"Last checked: %s", (strDate.IsEmpty() ? L"n/a" : strDate)));

	panel->SetSizerAndFit(sizer);

	return panel;
}

wxWindow *UpdateDlg::CreateSettingsPage(wxWindow *parent)
{
	wxPanel *panel = new wxPanel(parent);
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

	sizer->Add(hideCheck = new wxCheckBox(panel, wxID_ANY, L"Hide program to tray after start"), 0, wxALL | wxEXPAND, 5);
	sizer->Add(notifyCheck = new wxCheckBox(panel, wxID_ANY, L"Notify"), 0, wxALL | wxEXPAND, 5);
	sizer->Add(startCheck = new wxCheckBox(panel, wxID_ANY, L"Check for updates after start"), 0, wxALL | wxEXPAND, 5);
	sizer->Add(autoCheck = new wxCheckBox(panel, wxID_ANY, L"Check for updates automatically"), 0, wxALL | wxEXPAND, 5);

	wxStaticBoxSizer *acSizer = new wxStaticBoxSizer(wxHORIZONTAL, panel, L"Updates settings");
	wxStaticBox *updStaticBox = acSizer->GetStaticBox();
	{
		acSizer->Add(new wxStaticText(updStaticBox, wxID_ANY, L"Check for updates every"), 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5);
		acSizer->Add(updSpin = new wxSpinCtrl(updStaticBox, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 24), 0, wxALL | wxEXPAND, 5);
		acSizer->Add(new wxStaticText(updStaticBox, wxID_ANY, L"hour(s)"), 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);
	}

	wxCheckBox *targetCheck = autoCheck;

	autoCheck->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, [targetCheck, updStaticBox](wxCommandEvent &event)
	{
		updStaticBox->Enable(targetCheck->IsChecked());
		event.Skip();
	});

	sizer->Add(acSizer, 0, wxALL | wxEXPAND, 5);

	wxBoxSizer *subSizer = new wxBoxSizer(wxHORIZONTAL);
	{
		wxWindow *label;
		wxWindow *spin;

		subSizer->Add(targetCheck = atCheck = new wxCheckBox(panel, wxID_ANY, L"Automatically terminate target application after"), 0, wxALL | wxALIGN_CENTER_VERTICAL, 5);
		subSizer->Add(spin = atSpin = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 1, 120), 0, wxALL | wxEXPAND, 5);
		subSizer->Add(label = new wxStaticText(panel, wxID_ANY, L"second(s)"), 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5);

		atCheck->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, [targetCheck, label, spin] (wxCommandEvent &event)
		{
			spin->Enable(targetCheck->IsChecked());
			label->Enable(targetCheck->IsChecked());
			event.Skip();
		});
	}
	sizer->Add(subSizer, 0, wxALL | wxEXPAND, 5);

	Connect(wxID_ANY, wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler(UpdateDlg::OnSettingsCheckBox));
	Connect(wxID_ANY, wxEVT_COMMAND_SPINCTRL_UPDATED, wxSpinEventHandler(UpdateDlg::OnSettingsSpinCtrl));
	Connect(wxID_ANY, wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler(UpdateDlg::OnSettingsText));

	panel->SetSizerAndFit(sizer);

	return panel;
}

UpdateDlg::~UpdateDlg()
{
	SaveSettings();
}

void UpdateDlg::ShowUpdates()
{
	nbWidget->SetSelection(0);
	Show();
}

void UpdateDlg::ShowSettings()
{
	nbWidget->SetSelection(1);
	Show();
}

void UpdateDlg::OnOK(wxCommandEvent& event)
{
	OnApply(event);

	SaveSettings();
	Hide();
}

void UpdateDlg::OnCancel(wxCommandEvent& event)
{
	ApplyUpdateSettings(updateParams);
	EnableApplyButton(false);
	Hide();
}

void UpdateDlg::OnApply(wxCommandEvent& event)
{
	updateParams.showNotifications = notifyCheck->GetValue();
	updateParams.hideAfterStart = hideCheck->GetValue();
	updateParams.checkUpdatesAfterStart = startCheck->GetValue();
	updateParams.autoCheckUpdates = autoCheck->GetValue();
	updateParams.autoCheckInterval = updSpin->GetValue();
	updateParams.autoTerminateApp = atCheck->GetValue();
	updateParams.autoTerminateAppInterval = atSpin->GetValue();

	EnableApplyButton(false);
}

void UpdateDlg::EnableApplyButton(bool enable)
{
	wxButton *applyButton = dynamic_cast<wxButton *>(FindWindow(wxID_APPLY));
	if(applyButton) applyButton->Enable(enable);
}

void UpdateDlg::OnClose(wxCloseEvent &event)
{
	Hide();
}

void UpdateDlg::OnCheckThreadCompleted(CheckThreadEvent& event)
{
	wxWindow *waitWindow = FindWindow(WAIT_DLG);
	if(waitWindow) wxQueueEvent(FindWindow(WAIT_DLG), new wxCloseEvent(wxEVT_CLOSE_WINDOW, WAIT_DLG));

	const std::vector<UpdateInfo> &info = event.GetInfo();

	updListCtrl->DeleteAllItems();

	ready_to_autoupdate = ready_to_install = false;

	for(size_t i = 0; i < info.size(); ++i)
	{
		AppInfoProvider *infoProvider = (i == 0 ? static_cast<AppInfoProvider *>(&updProvider) : static_cast<AppInfoProvider *>(&appProvider));

		wxVector<wxVariant> list_row_data;

		if(!info[i].IsOk() || info[i].app_name != infoProvider->GetName())
		{
			if(!infoProvider->IsOk()) list_row_data.push_back(wxVariant(wxXmlResource::Get()->LoadBitmap(L"red_cross")));
			else list_row_data.push_back(wxVariant(wxBitmap()));

			list_row_data.push_back(wxVariant(infoProvider->GetName()));
			list_row_data.push_back(wxVariant(infoProvider->GetLocalVersion()));

			list_row_data.push_back(wxVariant(wxEmptyString));
			list_row_data.push_back(wxVariant(wxEmptyString));
		}
		else
		{
			if(!infoProvider->IsOk()) list_row_data.push_back(wxVariant(wxXmlResource::Get()->LoadBitmap(L"red_cross")));
			else
			{
				if(info[i].app_version > infoProvider->GetLocalVersion())
				{
					list_row_data.push_back(wxVariant(wxXmlResource::Get()->LoadBitmap(L"green_check")));

					ready_to_install = true;
					if(i == 0) ready_to_autoupdate = true;
				}
				else
				{
					list_row_data.push_back(wxVariant(wxBitmap()));
				}
			}

			wxString strSize = wxFileName::GetHumanReadableSize(wxULongLong(info[i].GetSize()), wxEmptyString);

			list_row_data.push_back(wxVariant(infoProvider->GetName()));
			list_row_data.push_back(wxVariant(infoProvider->GetLocalVersion()));

			list_row_data.push_back(wxVariant(info[i].app_version));
			list_row_data.push_back(wxVariant(strSize));
		}

		updListCtrl->AppendItem(list_row_data, 0);
	}

	wxString strDate = wxDateTime(wxDateTime::GetTimeNow()).FormatISOCombined(' ');
	lastUpdLabel->SetLabel(wxString::Format(L"Last checked: %s", strDate));

	wxConfigBase::Get()->Write(L"/LastChecked", strDate);

	updButton->Enable();
	installButton->Enable(ready_to_install);

	if(notifyCheck->GetValue())
	{
		wxString notify_text;

		if(ready_to_autoupdate && info.size() > 0) notify_text = wxString::Format(L"%s (%s -> %s)", updProvider.GetName(), updProvider.GetLocalVersion(), info[0].app_version);
		if(ready_to_install && info.size() > 1)
		{
			if(!notify_text.IsEmpty()) notify_text += L"\n";
			if(appProvider.GetLocalVersion().IsEmpty()) notify_text += wxString::Format(L"%s (%s)", appProvider.GetName(), info[1].app_version);
			else notify_text += wxString::Format(L"%s (%s -> %s)", appProvider.GetName(), appProvider.GetLocalVersion(), info[1].app_version);
		}

		wxCommandEvent *cmdEvent = new wxCommandEvent(wxEVT_SHOW_NOTIFICATION);
		cmdEvent->SetString(notify_text);

		wxQueueEvent(wxApp::GetInstance(), cmdEvent);		
	}
}

void UpdateDlg::CheckUpdates()
{
	wxLogMessage(L"Checking updates");

	if(!updProvider.IsOk()) {wxLogWarning(L"Updater provider is not OK for checking updates");}
	if(!appProvider.IsOk()) {wxLogWarning(L"Application provider is not OK for checking updates");}

	std::vector<wxString> urls = {updProvider.GetUpdateURL(), appProvider.GetUpdateURL()};

	CheckThread *thread = new CheckThread(this, urls);
	if(thread->Create() != wxTHREAD_NO_ERROR || thread->Run() != wxTHREAD_NO_ERROR)
	{
		wxLogError(L"Can not create check thread");
		return;
	}

	lastUpdLabel->SetLabel(L"Checking for updates...");
	updButton->Enable(false);
	RestartTimer(updateParams.autoCheckInterval);
}

void UpdateDlg::SetupTimer()
{
	timer = new wxTimer(this);
	//timer->Bind(wxEVT_TIMER, &UpdateDlg::OnUpdateTimer, this);
	Connect(timer->GetId(), wxEVT_TIMER, wxTimerEventHandler(UpdateDlg::OnUpdateTimer));
}

void UpdateDlg::RestartTimer(unsigned int hours)
{
	timer->Start(1000 * 3600 * hours);
	timerStartTime = wxDateTime::Now();

	//wxLogMessage(LTimer restarted to %d seconds", 3600 * hours);
}

void UpdateDlg::OnUpdateTimer(wxTimerEvent &event)
{
	wxLogMessage(L"Scheduled updates checking...");
	CheckUpdates();
}

void UpdateDlg::OnCheckUpdates(wxCommandEvent& event)
{
	CheckUpdates();

	WaitDlg dlg(this, WAIT_DLG, L"Checking for updates...");
	dlg.ShowModal();
}

void UpdateDlg::OnInstallUpdates(wxCommandEvent& event)
{
	if(ready_to_autoupdate)
	{
		if(wxMessageBox(L"Need to restart the program. Continue?", L"Update confirmation", wxYES | wxNO) == wxNO)
			return;

		wxQueueEvent(wxApp::GetInstance(), new wxCommandEvent(wxEVT_RESTART_AND_UPDATE));
		return;
	}

	if(ready_to_install)
	{
		if(!appProvider.IsOk())
		{
			wxLogWarning(L"Application provider is not OK for installing updates");
			return;
		}

		DownloadDlg dlg(this, DOWNLOAD_DLG, &appProvider, L"Installing updates");
		int res = dlg.ShowModal();

		if(res == DownloadDlg::UPDATE_OK)
		{
			wxMessageBox(L"Update(s) successfully installed", L"Information");
			installButton->Enable(false);

			appProvider.Reset();
			CheckUpdates();
		}
		else if(res == DownloadDlg::UPDATE_CANCEL) wxMessageBox(L"Update canceled", L"Information", wxICON_WARNING | wxOK | wxCENTER);
		else if(res == DownloadDlg::UPDATE_FAIL) wxMessageBox(L"Update failed", L"Information", wxICON_ERROR | wxOK | wxCENTER);

	}
}

void UpdateDlg::OnSettingsCheckBox(wxCommandEvent &event)
{
	int id = event.GetId();

	if(id == hideCheck->GetId() || id == autoCheck->GetId() || id == notifyCheck->GetId() || id == startCheck->GetId() || id == atCheck->GetId())
		EnableApplyButton(TestUpdateSettingsForChanges());

	event.Skip();
}

void UpdateDlg::OnSettingsSpinCtrl(wxSpinEvent& event)
{
	int id = event.GetId();

	if(id == updSpin->GetId() || id == atSpin->GetId()) EnableApplyButton(TestUpdateSettingsForChanges());

	event.Skip();
}

void UpdateDlg::OnSettingsText(wxCommandEvent& event)
{
	int id = event.GetId();

	if(id == updSpin->GetId() || id == atSpin->GetId()) EnableApplyButton(TestUpdateSettingsForChanges());

	event.Skip();
}

bool UpdateDlg::HideAfterStart() const
{
	return updateParams.hideAfterStart;
}

wxTimeSpan UpdateDlg::GetTimeToNextUpdate() const
{
	wxDateTime updateTime = timerStartTime.Add(wxTimeSpan(updateParams.autoCheckInterval));
	wxDateTime currentTime = wxDateTime::Now();

	if(!currentTime.IsEarlierThan(updateTime))
	{
		wxLogWarning(L"Strange time");
		return wxTimeSpan();
	}

	return updateTime.Subtract(currentTime);
}

void UpdateDlg::LoadSettings()
{
	LoadUpdateSettings(updateParams);
	ApplyUpdateSettings(updateParams);

	LoadDialogSettings();
}

void UpdateDlg::LoadUpdateSettings(UpdateParams &params)
{
	wxConfigBase *cfg = wxConfigBase::Get();

	params.showNotifications = ReadConfigValue(cfg, L"/ShowNotifications", true);
	params.hideAfterStart = ReadConfigValue(cfg, L"/HideAfterStart", false);
	params.checkUpdatesAfterStart = ReadConfigValue(cfg, L"/CheckUpdatesAfterStart", true);
	params.autoCheckUpdates = ReadConfigValue(cfg, L"/AutoCheckUpdates", true);
	params.autoCheckInterval = ReadConfigValue(cfg, L"/AutoCheckInterval", 5);
	params.autoTerminateApp = ReadConfigValue(cfg, L"/AutoTerminateApp", true);
	params.autoTerminateAppInterval = ReadConfigValue(cfg, L"/AutoTerminateAppInterval", 30);
}

void UpdateDlg::ApplyUpdateSettings(const UpdateParams &params)
{
	notifyCheck->SetValue(params.showNotifications);
	hideCheck->SetValue(params.hideAfterStart);
	startCheck->SetValue(params.checkUpdatesAfterStart);
	autoCheck->SetValue(params.autoCheckUpdates);
	updSpin->SetValue(params.autoCheckInterval);
	atCheck->SetValue(params.autoTerminateApp);
	atSpin->SetValue(params.autoTerminateAppInterval);

	wxQueueEvent(autoCheck, new wxCommandEvent(wxEVT_COMMAND_CHECKBOX_CLICKED, autoCheck->GetId()));
	wxQueueEvent(atCheck, new wxCommandEvent(wxEVT_COMMAND_CHECKBOX_CLICKED, atCheck->GetId()));

	RestartTimer(updSpin->GetValue());
}

bool UpdateDlg::TestUpdateSettingsForChanges()
{
	if(	notifyCheck->GetValue() != updateParams.showNotifications ||
		hideCheck->GetValue() != updateParams.hideAfterStart ||
		startCheck->GetValue() != updateParams.checkUpdatesAfterStart ||
		autoCheck->GetValue() != updateParams.autoCheckUpdates ||
		updSpin->GetValue() != static_cast<int>(updateParams.autoCheckInterval) ||
		atCheck->GetValue() != updateParams.autoTerminateApp ||
		atSpin->GetValue() != static_cast<int>(updateParams.autoTerminateAppInterval)) return true;

	return false;
}

void UpdateDlg::LoadDialogSettings()
{
	wxConfigBase *cfg = wxConfigBase::Get();

	int width, height, left, top;

	cfg->Read(L"/UpdateDlg/Width", &width, wxDefaultCoord);
	cfg->Read(L"/UpdateDlg/Height", &height, wxDefaultCoord);
	cfg->Read(L"/UpdateDlg/Left", &left, wxDefaultCoord);
	cfg->Read(L"/UpdateDlg/Top", &top, wxDefaultCoord);

	SetSize(width, height);
	Move(left, top);

	for(size_t i = 0; i < updListCtrl->GetColumnCount(); i++)
	{
		wxDataViewColumn *column = updListCtrl->GetColumn(i);

		int width;
		if(cfg->Read(L"/UpdateDlg/UpdListHeader/" + columnsIds[i], &width)) column->SetWidth(width);
	}	
}

void UpdateDlg::SaveSettings()
{
	SaveUpdateSettings(updateParams);
	SaveDialogSettings();

	wxConfigBase::Get()->Flush();
}

void UpdateDlg::SaveUpdateSettings(const UpdateParams &params)
{
	wxConfigBase *cfg = wxConfigBase::Get();

	cfg->Write(L"/ShowNotifications", params.showNotifications);
	cfg->Write(L"/HideAfterStart", params.hideAfterStart);
	cfg->Write(L"/CheckUpdatesAfterStart", params.checkUpdatesAfterStart);
	cfg->Write(L"/AutoCheckUpdates", params.autoCheckUpdates);
	cfg->Write(L"/AutoCheckInterval", params.autoCheckInterval);
	cfg->Write(L"/AutoTerminateApp", params.autoTerminateApp);
	cfg->Write(L"/AutoTerminateAppInterval", params.autoTerminateAppInterval);
}

void UpdateDlg::SaveDialogSettings()
{
	wxConfigBase *cfg = wxConfigBase::Get();

	cfg->Write(L"/UpdateDlg/Width", GetSize().GetWidth());
	cfg->Write(L"/UpdateDlg/Height", GetSize().GetHeight());
	cfg->Write(L"/UpdateDlg/Left", GetPosition().x);
	cfg->Write(L"/UpdateDlg/Top", GetPosition().y);

	for(size_t i = 0; i < updListCtrl->GetColumnCount(); i++)
	{
		wxDataViewColumn *column = updListCtrl->GetColumn(i);
		cfg->Write(L"/UpdateDlg/UpdListHeader/" + columnsIds[i], column->GetWidth());
	}
}