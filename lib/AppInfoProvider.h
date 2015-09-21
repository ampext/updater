#pragma once

#include <vector>
#include <map>
#include <wx/string.h>

class AppInfoProvider
{
public:
	virtual wxString GetName() const = 0;
	virtual wxString GetProcName() const = 0;
	virtual wxString GetLocalVersion() const = 0;
	virtual bool IsOk() const = 0;
	virtual wxString GetInstallDir() const = 0;

	virtual wxString GetUpdateURL() const = 0;

	bool operator !() const	{ return !IsOk(); }
};

class UpdaterProvider : public AppInfoProvider
{
private:
	wxString installPath;
	wxString updateURL;

public:
	UpdaterProvider(const wxString &installPath = wxEmptyString, const wxString &updateURL = wxEmptyString);
	UpdaterProvider(const std::map<wxString, wxString> &props);

	wxString GetName() const;
	wxString GetProcName() const;
	wxString GetLocalVersion() const;
	bool IsOk() const;
	wxString GetInstallDir() const;

	wxString GetUpdateURL() const;
};

class TargetAppProvider : public AppInfoProvider
{
public:
	TargetAppProvider() {}
	TargetAppProvider(const std::map<wxString, wxString> &props, const wxString &targetName = wxEmptyString);

	wxString GetName() const;
	wxString GetProcName() const;
	wxString GetLocalVersion() const;
	bool IsOk() const;
	wxString GetInstallDir() const;

	wxString GetUpdateURL() const;

private:
	wxString appName;
	wxString procName;
	wxString installDir;
	wxString updateURL;
};

bool ReadProviders(UpdaterProvider &updProvider, std::vector<TargetAppProvider> &targetProviders);