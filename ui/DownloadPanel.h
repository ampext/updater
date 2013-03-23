#pragma once

#include <wx/panel.h>
#include <wx/gauge.h>
#include <wx/stattext.h>

class DownloadPanel: public wxPanel
{
public:
	DownloadPanel(wxWindow *parent);
	void SetInfo(const wxString &action, const wxString &status, unsigned int progress);

private:
	wxStaticText *actionLabel;
	wxStaticText *statusLabel;
	wxGauge *progressCtrl;
};