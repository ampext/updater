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
	virtual void Reset() {};

protected:
	std::map<wxString, wxString> GetProperties(const wxString& name);
};

class UpdaterProvider : public AppInfoProvider
{
private:
	wxString installPath;
	wxString updateURL;

public:
	UpdaterProvider(const wxString &installDir = wxEmptyString, const wxString &updateURL = wxEmptyString);

	wxString GetName() const;
	wxString GetProcName() const;
	wxString GetLocalVersion() const;
	bool IsOk() const;
	wxString GetInstallDir() const;
	void Reset();

	wxString GetUpdateURL() const;
};

class TargetAppProvider : public AppInfoProvider
{
public:
	TargetAppProvider();

	wxString GetName() const;
	wxString GetProcName() const;
	wxString GetLocalVersion() const;
	bool IsOk() const;
	wxString GetInstallDir() const;
	void Reset();

	wxString GetUpdateURL() const;

private:
	wxString appName;
	wxString procName;
	wxString installDir;
	wxString updateURL;
};