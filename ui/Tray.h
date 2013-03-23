#pragma once

#include <wx/taskbar.h>
#include <wx/menu.h>

#include <functional>

typedef std::function<wxMenu *()> MenuCreator;

class TrayIcon : public wxTaskBarIcon
{
private:
	MenuCreator menuCreator;
	
private:
	virtual wxMenu *CreatePopupMenu();

public:
    TrayIcon(const MenuCreator &creator);
};