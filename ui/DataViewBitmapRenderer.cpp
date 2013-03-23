#include "DataViewBitmapRenderer.h"

#include <wx/dc.h>

DataViewBitmapRenderer::DataViewBitmapRenderer() : wxDataViewRenderer(L"wxBitmap", wxDATAVIEW_CELL_INERT, wxDVR_DEFAULT_ALIGNMENT)
{

}

bool DataViewBitmapRenderer::SetValue(const wxVariant &value)
{
    if(value.GetType() == L"wxBitmap")
    {
    	bitmap << value;
    	return true;
    }

    return false;
}

bool DataViewBitmapRenderer::GetValue(wxVariant& value) const
{
    return false;
}

bool DataViewBitmapRenderer::Render(wxRect cell, wxDC *dc, int state)
{
    if(bitmap.IsOk())
    {
    	int y = cell.y;

    	// center vertivel align
    	if(cell.height > bitmap.GetHeight()) y += 0.5 * (cell.height - bitmap.GetHeight());

        dc->DrawBitmap(bitmap, cell.x, y);
        return true;
    }

    return false;
}

wxSize DataViewBitmapRenderer::GetSize() const
{
    if(bitmap.IsOk()) return bitmap.GetSize();
    return wxSize(wxDVC_DEFAULT_RENDERER_SIZE, wxDVC_DEFAULT_RENDERER_SIZE);
}