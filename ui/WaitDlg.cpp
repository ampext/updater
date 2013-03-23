#include "WaitDlg.h"

#include <wx/sizer.h>
#include <wx/stattext.h>
#include <iostream>

WaitDlg::WaitDlg(wxWindow *parent, wxWindowID id, const wxString &text, const wxString &title) : wxDialog(parent, id, title, wxDefaultPosition, wxDefaultSize, wxCAPTION)
{
	wxBoxSizer *sizer = new wxBoxSizer(wxVERTICAL);

	sizer->Add(new wxStaticText(this, wxID_ANY, text), 1, wxALL | wxEXPAND, 5);

	SetSizerAndFit(sizer);
}

void WaitDlg::OnThreadCompleted(wxThreadEvent &event)
{
	
}