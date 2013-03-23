#pragma once

#include <exception>
#include <wx/fileconf.h>

class Exception : public std::exception
{
	wxString desc;
	
public:
	
	Exception(const wxString &desc) : desc(desc)
	{
		
	}
	
	~Exception() throw()
	{
		
	}
	
	wxString GetDescription() const
	{
		return desc;
	}
};

template <class T> T ReadConfigValue(const wxString &path, const T &def_value)
{
	return ReadConfigValue(wxConfigBase::Get(), path, def_value);
}

template <class T> T ReadConfigValue(wxConfigBase *cfg, const wxString &path, const T &def_value)
{
	T value;

	if(!cfg->Read(path, &value, def_value))
		cfg->Write(path, def_value);

	return value;	
}