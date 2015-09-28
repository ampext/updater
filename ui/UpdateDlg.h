#pragma once

#include "lib/AppInfoProvider.h"
#include "UpdateThreads.h"

#include <wx/timer.h>
#include <wx/dialog.h>
#include <wx/notebook.h>
#include <wx/checkbox.h>
#include <wx/checkbox.h>
#include <wx/dataview.h>
#include <wx/stattext.h>
#include <wx/button.h>
#include <wx/statbox.h>
#include <wx/spinctrl.h>

class UpdateDlg: public wxDialog
{
public:
	UpdateDlg(wxWindow *parent = nullptr);
	virtual ~UpdateDlg();

	void ShowSettings();
	void ShowUpdates();

	bool HideAfterStart() const;
	wxTimeSpan GetTimeToNextUpdate() const;

private:

	struct UpdateParams
	{
		bool showNotifications;
		bool hideAfterStart;
		bool checkUpdatesAfterStart;
		bool autoCheckUpdates;
		unsigned int autoCheckInterval;
		bool autoTerminateApp;
		unsigned int autoTerminateAppInterval;
		bool logPermissions;

		UpdateParams()
		{
			showNotifications = false;
			hideAfterStart = false;
			checkUpdatesAfterStart = false;
			autoCheckUpdates = false;
			autoCheckInterval = 0;
			autoTerminateApp = true;
			autoTerminateAppInterval = 30;
			logPermissions = false;
		}

	} updateParams;

private:
	wxWindow *CreateUpdatesPage(wxWindow *parent);
	wxWindow *CreateSettingsPage(wxWindow *parent);

	void OnOK(wxCommandEvent& event);
	void OnCancel(wxCommandEvent& event);
	void OnApply(wxCommandEvent& event);
	void OnClose(wxCloseEvent &event);

	void OnAutoCheckUpdates(wxCommandEvent &event);
	void OnCheckThreadCompleted(CheckThreadEvent& event);
	void OnCheckUpdates(wxCommandEvent& event);
	void OnInstallUpdate(wxCommandEvent& event);
	void OnInstallAllUpdates(wxCommandEvent& event);
	void OnUpdateTimer(wxTimerEvent &event);
	void OnSettingsCheckBox(wxCommandEvent &event);
	void OnSettingsSpinCtrl(wxSpinEvent& event);
	void OnSettingsText(wxCommandEvent& event);

	void OnUpdateListSelectionChanged(wxDataViewEvent& event);
	
	void EnableApplyButton(bool enable);
	bool TestUpdateSettingsForChanges();
	void SetupTimer();
	void RestartTimer(unsigned int hours);
	void CheckUpdates();
	bool CheckForRestart();

	void LoadSettings();
	void LoadUpdateSettings(UpdateParams &params);
	void LoadDialogSettings();

	void SaveSettings();
	void SaveUpdateSettings(const UpdateParams &params);
	void SaveDialogSettings();
	void ApplyUpdateSettings(const UpdateParams &params);

	bool CheckAppProvidersStatus() const;
	bool HasAppUpdate(size_t index) const;
	bool HasAppUpdates() const;

	bool DoInstall(const AppInfoProvider &appProvider);

	wxTimer *timer;
	wxDateTime timerStartTime;

	wxNotebook	*nbWidget;
	wxDataViewListCtrl *updListCtrl;
	wxStaticText *lastUpdLabel;
	wxButton *updButton;
	wxButton *installButton;
	wxButton *installAllButton;
	wxCheckBox *hideCheck;
	wxCheckBox *notifyCheck;
	wxCheckBox *autoCheck;
	wxCheckBox *startCheck;
	wxCheckBox *atCheck;
	wxSpinCtrl *updSpin;
	wxSpinCtrl *atSpin;
	wxCheckBox *permCheck;

	UpdaterProvider updProvider;
	std::vector<TargetAppProvider> appProviders;

	std::vector<bool> readyToUpdate;

	std::vector<wxString> columnsIds;
};