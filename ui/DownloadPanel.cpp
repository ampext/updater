#include "DownloadPanel.h"

#include <wx/sizer.h>

DownloadPanel::DownloadPanel(wxWindow *parent) : wxPanel(parent, wxID_ANY)
{
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

	sizer->Add(actionLabel = new wxStaticText(this, wxID_ANY, L"ACTION"), 0, wxALL | wxEXPAND);
	sizer->AddSpacer(5);
	sizer->Add(statusLabel = new wxStaticText(this, wxID_ANY, L"STATUS"), 0, wxALL | wxEXPAND);
	sizer->AddSpacer(5);
	sizer->Add(progressCtrl = new wxGauge(this, wxID_ANY, 100), 0, wxALL | wxEXPAND);

	wxFont font = actionLabel->GetFont();
	font.SetWeight(wxFONTWEIGHT_BOLD);
	font.SetPointSize(font.GetPointSize() + 2);

	actionLabel->SetFont(font);

	SetSizerAndFit(sizer);
}

void DownloadPanel::SetInfo(const wxString &action, const wxString &status, unsigned int progress)
{
	actionLabel->SetLabel(action);
	statusLabel->SetLabel(status);
	progressCtrl->SetValue(std::min<int>(progress, 100));
}