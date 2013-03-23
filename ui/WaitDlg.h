#pragma once

#include <wx/dialog.h>

class WaitDlg: public wxDialog
{
public:
	WaitDlg(wxWindow *parent = nullptr, wxWindowID id = wxID_ANY, const wxString &text = L"Wait", const wxString &title = L"Wait"); 
	void OnThreadCompleted(wxThreadEvent &event);
};