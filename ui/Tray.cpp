#include "Tray.h"

#include <wx/xrc/xmlres.h>
#include <wx/log.h>

TrayIcon::TrayIcon(const MenuCreator &creator) : menuCreator(creator)
{
	wxIcon icon;
	icon.CopyFromBitmap(wxXmlResource::Get()->LoadBitmap(L"tray"));

	SetIcon(icon);
}

wxMenu *TrayIcon::CreatePopupMenu()
{
	if(menuCreator) return menuCreator();
	return nullptr;
}