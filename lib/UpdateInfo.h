#pragma once

#include <memory>
#include <vector>
#include <numeric>

#include <wx/string.h>
#include <wx/longlong.h>

struct UpdateFile
{
	wxString src;
	wxString hash;
	wxULongLong_t size;
	
	UpdateFile() : size(0L)
	{

	}
};

struct UpdateElement
{
	wxString type;
	std::vector<UpdateFile> files;

	wxULongLong_t GetSize() const
	{
		return std::accumulate(files.begin(), files.end(), 0L, [] (wxULongLong_t res, const UpdateFile &file)
		{
			return res + file.size;
		});
	}
};

struct UpdateInfo
{
	int format_version;
	
	wxString app_name;
	wxString app_version;
	wxString process_name;
	wxString info;
	
	std::vector<std::shared_ptr<UpdateElement> > elements;
	
	UpdateInfo() : format_version(0)
	{
		
	}

	void Clear()
	{
		app_version = app_version = process_name = info = wxEmptyString;
		elements.clear();
	}

	bool IsOk() const
	{
		return !app_name.IsEmpty() && !app_version.IsEmpty() && !process_name.IsEmpty();
	}

	bool IsEmpty() const
	{
		return elements.empty() || elements[0]->files.empty();
	}

	wxULongLong_t GetSize() const
	{
		return std::accumulate(elements.begin(), elements.end(), 0L, [] (wxULongLong_t res, const std::shared_ptr<UpdateElement> &element)
		{
			return res + element->GetSize();
		});
	}
};