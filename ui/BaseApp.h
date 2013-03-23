#pragma once

#define wxUSE_SNGLINST_CHECKER 1

#include <wx/snglinst.h>
#include <wx/fileconf.h>
#include <wx/stdpaths.h>
#include <wx/log.h>

#include <fstream>
#include <memory>

class FileLogFormatter: public wxLogFormatter
{
public:
	virtual wxString Format(wxLogLevel level, const wxString& msg, const wxLogRecordInfo& info) const
	{
		return wxString::Format("[%s][%s]: %s", FormatTime(info.timestamp), info.component, msg);
	}

	virtual wxString FormatTime(time_t time) const
	{
		return wxDateTime(time).FormatISOCombined(' ');
	}
};

class UiLogFormatter: public wxLogFormatter
{
public:
	virtual wxString Format(wxLogLevel level, const wxString& msg, const wxLogRecordInfo& info) const
	{
		return wxString::Format("[%s][%s]: %s", FormatTime(info.timestamp), info.component, msg);
	}

	virtual wxString FormatTime(time_t time) const
	{
		return wxDateTime(time).FormatISOTime();
	}
};

class BaseApp
{
public:
	BaseApp()
	{

	}

protected:
	void SetupLogger()
	{
		wxLog *fileLogger = nullptr;
		
		logStream.open(GetLogFilePath().ToStdString().c_str(), std::ofstream::out | std::ofstream::app);

		if(logStream.is_open() && logStream.good())
		{
			fileLogger = new wxLogStream(static_cast<std::ostream *>(&logStream));
			fileLogger->SetFormatter(new FileLogFormatter);
		}
		else wxLogError(L"Can not open log file '" + GetLogFilePath() + "'");

		wxLog *errLogger = new wxLogStderr();
		errLogger->SetFormatter(new UiLogFormatter);
		wxLog::SetActiveTarget(errLogger);
		wxLog::SetLogLevel(wxLOG_Info);

		if(fileLogger)
			new wxLogChain(fileLogger);

		wxLogMessage(L"Logging started");
	}

	bool CheckSingleInstance()
	{
		instanceChecker.reset(new wxSingleInstanceChecker());

		if(instanceChecker->IsAnotherRunning())
		{
			instanceChecker.reset();
			return false;
		}

		return true;
	}

	void SetupConfig()
	{
		if(!wxDirExists(GetSettingsPath()))
		{
			wxLogMessage(L"Creating settings path '" + GetSettingsPath() + "'");
			wxMkDir(GetSettingsPath(), wxS_DIR_DEFAULT);
		}

		wxLogMessage(L"Config path '" + GetSettingsFilePath() + "'");

		wxFileConfig *pConfig = new wxFileConfig(L"Updater Client", L"Ololo Corp.", GetSettingsFilePath(), wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
		if(pConfig) wxConfigBase::Set(pConfig);
		else wxLogError("Can't create config");
	}

	void OnExit(bool close_log = true)
	{
		wxLogMessage(L"Exiting...");

		if(logStream.is_open()) logStream.close();

		delete wxConfigBase::Set(NULL);

		instanceChecker.reset();
	}

public:

	static wxString GetSettingsPath()
	{
		#ifdef __WXMSW__
			return wxStandardPaths::Get().GetUserConfigDir() + wxFileName::GetPathSeparator() + L"updater";
		#else
			return wxStandardPaths::Get().GetUserConfigDir() + wxFileName::GetPathSeparator() + L".config" + wxFileName::GetPathSeparator() + L"updater";
		#endif
	}

	static wxString GetSettingsFilePath()
	{
		return GetSettingsPath() + wxFileName::GetPathSeparator() + L"settings";
	}

	static wxString GetLogFilePath()
	{
		return GetSettingsPath() + wxFileName::GetPathSeparator() + L"log";
	}

protected:
	std::ofstream logStream;
	std::unique_ptr<wxSingleInstanceChecker> instanceChecker;
};