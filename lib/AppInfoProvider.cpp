#include "AppInfoProvider.h"
#include "Utils.h"

#include <wx/stdpaths.h>
#include <wx/config.h>
#include <wx/wfstream.h>
#include <wx/log.h>
#include <wx/xml/xml.h>

#define wxLOG_COMPONENT "updater"

std::map<wxString, wxString> ReadProperties(wxXmlNode *appNode)
{
	std::map<wxString, wxString> props;

	wxXmlNode *propertyNode = appNode->GetChildren();

	while(propertyNode)
	{
		if(propertyNode->GetName() == L"property")
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
		}

		propertyNode = propertyNode->GetNext();
	}

	return props;
}

bool ReadProviders(UpdaterProvider &updProvider, std::vector<TargetAppProvider> &targetProviders)
{
	wxFileName updFileName(wxStandardPaths::Get().GetExecutablePath());
	wxFileName providersFileName(updFileName.GetPath() + wxFileName::GetPathSeparator() + L"providers.xml");

	targetProviders.clear();

	try
	{
		wxFileInputStream stream(providersFileName.GetFullPath());
		if(!stream)	throw Exception(wxString::Format("Can't find '%s'", providersFileName.GetFullPath()));

		wxXmlDocument doc(stream);

		wxXmlNode *node = doc.GetRoot();
		if(!node || node->GetName() != L"providers") throw Exception(L"providers node not found");

		wxXmlNode *appNode = node->GetChildren();
		
		while(appNode)
		{
			if(appNode->GetName() == L"app")
			{
				wxString app_name = appNode->GetAttribute(L"name");
				std::map<wxString, wxString> props = ReadProperties(appNode);

				if(app_name == "Updater") updProvider = UpdaterProvider(props);
				else targetProviders.push_back(TargetAppProvider(props, app_name));
			}

			appNode = appNode->GetNext();
		}
	}
	catch(Exception &e)
	{
		wxLogError(e.GetDescription());
		return false;
	}

	return true;
}

UpdaterProvider::UpdaterProvider(const wxString &installPath, const wxString &updateURL): installPath(installPath), updateURL(updateURL)
{

}

UpdaterProvider::UpdaterProvider(const std::map<wxString, wxString> &props)
{
	if(updateURL.IsEmpty())
	{
		if(props.count(L"update_url")) this->updateURL = props.find(L"update_url")->second;
		else
		{
			wxLogError(L"Can not find '%s' property for updater", L"update_url");
		}
	}
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
		return wxString::Format(L"%s.%d", base_version, BUILD_NUMBER);
	#else
		return base_version;
	#endif

	return wxEmptyString;
}

bool UpdaterProvider::IsOk() const
{
	return !updateURL.IsEmpty();
}

wxString UpdaterProvider::GetInstallDir() const
{
	return installPath;
}

wxString UpdaterProvider::GetUpdateURL() const
{
	return updateURL;
}

TargetAppProvider::TargetAppProvider(const std::map<wxString, wxString> &props, const  wxString &appName): appName(appName)
{
	auto readProperty = [props](const wxString &name)
	{
		auto it =  props.find(name);
		if(it == props.end()) throw Exception(name);

		return it->second;
	};

	try
	{
		procName = readProperty(L"process_name");
		updateURL = readProperty(L"update_url");
		installDir = readProperty(L"install_dir");
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
	wxConfigBase::Get()->Read(L"/Software/" + GetName() + L"/Version", &version, wxEmptyString);
	
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