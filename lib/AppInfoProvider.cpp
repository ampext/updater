#include "AppInfoProvider.h"
#include "Utils.h"

#include <wx/stdpaths.h>
#include <wx/config.h>
#include <wx/wfstream.h>
#include <wx/log.h>
#include <wx/xml/xml.h>

#define wxLOG_COMPONENT "updater"

std::map<wxString, wxString> AppInfoProvider::GetProperties(const wxString& name)
{
	std::map<wxString, wxString> props;

	wxFileName updFileName(wxStandardPaths().GetExecutablePath());
	wxFileName providersFileName(updFileName.GetPath() + wxFileName::GetPathSeparator() + L"providers.xml");

	try
	{
		wxFileInputStream stream(providersFileName.GetFullPath());
		if(!stream)	throw Exception(wxString::Format("Can't find '%s'", providersFileName.GetFullPath()));

		wxXmlDocument doc(stream);

		wxXmlNode *node = doc.GetRoot();
		if(!node || node->GetName() != L"providers") throw Exception(L"providers node not found");

		wxXmlNode *appNode = node->GetChildren();
		
		while(appNode && appNode->GetName() == L"app")
		{
			wxString app_name;
			appNode->GetAttribute(L"name", &app_name);

			if(app_name == name)
			{
				wxXmlNode *propertyNode = appNode->GetChildren();

				while(propertyNode && propertyNode->GetName() == L"property")
				{
					wxString prop_name;
					wxString prop_value;

					propertyNode->GetAttribute(L"name", &prop_name);
					propertyNode->GetAttribute(L"value", &prop_value);

					if(prop_name.IsEmpty())
					{
						wxLogWarning(L"Empty property name at %d line", propertyNode->GetLineNumber());
					}

					props[prop_name] = prop_value;

					propertyNode = propertyNode->GetNext();
				}
			}
			
			appNode = appNode->GetNext();
		}
	}
	catch(Exception &e)
	{
		wxLogError(e.GetDescription());
	}

	return props;
}

UpdaterProvider::UpdaterProvider(const wxString &installPath, const wxString &updateURL) : installPath(installPath), updateURL(updateURL)
{
	if(updateURL.IsEmpty())
	{
		std::map<wxString, wxString> props = GetProperties(UpdaterProvider::GetName());

		if(props.count(L"update_url")) this->updateURL = props[L"update_url"];
		else
		{
			wxLogError(L"Can not find '%s' property for updater", L"update_url");
		}
	}
}

void UpdaterProvider::Reset()
{

}

wxString UpdaterProvider::GetName() const
{
	return L"Updater";
}

wxString UpdaterProvider::GetProcName() const
{
	return L"upd_ui.exe";
}

wxString UpdaterProvider::GetLocalVersion() const
{
	wxString base_version = L"0.1";

	#ifdef BUILD_NUMBER
		return wxString::Format("%s.%d", base_version, BUILD_NUMBER);
	#else
		return base_version;
	#endif

	return wxEmptyString;
}

bool UpdaterProvider::IsOk() const
{
	return true;
}

wxString UpdaterProvider::GetInstallDir() const
{
	return installPath;
}

wxString UpdaterProvider::GetUpdateURL() const
{
	return updateURL;
}

TargetAppProvider::TargetAppProvider()
{
	Reset();
}

void TargetAppProvider::Reset()
{
	std::map<wxString, wxString> props = GetProperties(L"Application");

	try
	{
		if(props.count(L"process_name")) procName = props[L"process_name"];
		else throw Exception(L"process_name");

		if(props.count(L"update_url")) updateURL = props[L"update_url"];
		else throw Exception(L"update_url");

		if(props.count(L"install_dir")) installDir = props[L"install_dir"];
		else throw Exception(L"install_dir");

		if(props.count(L"app_name")) appName = props[L"app_name"];
		else appName = L"Application";
	}
	catch(Exception &e)
	{
		wxLogError(L"Can not find '%s' property for application", e.GetDescription());
	}	

	wxFileName instFileName(installDir);
	
	if(instFileName.Normalize(wxPATH_NORM_ENV_VARS)) installDir = instFileName.GetFullPath();
	else
	{
		wxLogWarning(L"Can not normalize app installation path '%s'", installDir);
	}
}

wxString TargetAppProvider::GetName() const
{
	return appName;
}

wxString TargetAppProvider::GetProcName() const
{
	return procName;
}

wxString TargetAppProvider::GetLocalVersion() const
{
	wxString version;
	wxConfigBase::Get()->Read(L"/Software/" + GetName() + "/Version", &version, wxEmptyString);
	
	return version;
}

bool TargetAppProvider::IsOk() const
{
	return wxFileExists(GetInstallDir() +  wxFileName::GetPathSeparator() + GetProcName());
}

wxString TargetAppProvider::GetInstallDir() const
{
	return installDir;
}

wxString TargetAppProvider::GetUpdateURL() const
{
	return updateURL;
}