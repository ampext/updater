#pragma once

#include <wx/dataview.h>

class DataViewBitmapRenderer: public wxDataViewRenderer
{
public:
    DataViewBitmapRenderer();

    bool SetValue(const wxVariant &value);
    bool GetValue(wxVariant &value) const;

    bool Render(wxRect cell, wxDC *dc, int state);
    wxSize GetSize() const;

private:
    wxBitmap bitmap;
};