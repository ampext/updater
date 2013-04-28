#include "Updater.h"

#include <iostream>
#include <sstream>
#include <array>

#include <wx/log.h>
#include <wx/sstream.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>
#include <wx/filename.h>
#include <wx/fileconf.h>
#include <wx/regex.h>

#include <boost/uuid/sha1.hpp>
#include "Url.h"

#define wxLOG_COMPONENT "updater"

Updater::Updater(const wxString &url) : baseURL(url)
{

}

void Updater::SetBaseURL(const wxString &url)
{
	baseURL = url;
}

wxString Updater::GetBaseURL() const
{
	return baseURL;
}

bool Updater::DownloadUpdate(const UpdateInfo &info, wxString &updFileName)
{
	if(info.elements.empty()) return false;
	if(info.elements[0]->files.empty() || info.elements[0]->files[0].src.empty()) return false;

	wxString tmpFilePath = wxFileName::GetTempDir() + wxFileName::GetPathSeparator() + L"updater" + wxFileName::GetPathSeparator();
	wxString tmpFileName = wxFileName::CreateTempFileName(tmpFilePath);

	auto downloadFunc = [&](wxLongLong_t value, wxLongLong_t size, wxLongLong_t speed) -> bool
	{
		if(progressFunc) return progressFunc(UpdateDownload, value, size, speed, wxEmptyString);
		return false;
	};

	try
	{
		if(!wxDirExists(tmpFilePath))
		{
			wxLogMessage(L"Creating path '%s'", tmpFilePath);
			wxMkDir(tmpFilePath, wxS_DIR_DEFAULT);
		}

		wxRegEx startUri(L"^([[:alpha:]]+://)");

		wxString url = info.elements[0]->files[0].src;
		if(!startUri.Matches(url)) url = baseURL + L"/" + url;

		wxLogMessage(L"Downloading '%s'", url);

		if(!DownloadFile(url, tmpFileName, downloadFunc)) throw std::exception();
	}
	catch(std::exception &e)
	{
		wxLogError(L"Download failed");
		wxRemoveFile(tmpFileName);
		return false;
	}

	const wxString &hash = info.elements[0]->files[0].hash;

	if(!hash.IsEmpty())
	{
		progressFunc(UpdateCheckSum, 0, 0, 0, wxEmptyString);

		wxString fileHash = GetFileSHA1(tmpFileName);

		if(hash.Lower() != fileHash)
		{
			wxLogError(L"Check sum verification failed (%s != %s)", hash.Lower(), fileHash.Lower());
			return false;
		}
	}
	else wxLogWarning(L"Empty hash, skipping chesk sum verification");

	updFileName = tmpFileName;
	return true;
}

bool Updater::InstallUpdate(const wxString &updFileName, const wxString &dstPath)
{
	auto installFunc = [&](wxLongLong_t count, wxLongLong_t total, const wxString &name) -> bool
	{
		if(progressFunc) return progressFunc(UpdateInstall, count, total, 0, name);
		return false;
	};

	try
	{
		wxLogMessage(L"Extracting file(s) to '%s'", dstPath);
		if(!ExtractFilesFromZip(updFileName, dstPath, installFunc)) throw std::exception();
	}
	catch(std::exception &e)
	{
		wxLogError(L"Installation failed");
		wxRemoveFile(updFileName);
		return false;
	}

	wxRemoveFile(updFileName);
	return true;
}

bool Updater::UpdateVersionInfo(const UpdateInfo &info)
{
	if(info.app_name.IsEmpty() || info.app_version.IsEmpty()) return false;

	wxLogMessage(L"Updating local version");

	wxConfigBase::Get()->Write(L"/Software/" + info.app_name + "/Version", info.app_version);
	return true;
}

wxString Updater::GetUpdateInfo(const wxString &name) const
{
	UrlHelper url(baseURL + name);
	
	if(!url.IsOk())
	{
		wxLogError(L"Can not open URL '%s' - %s", url.GetUrl(), url.GetError());
	}

	wxLogMessage(L"Fetching update info from '%s'", baseURL + name);

	wxStringOutputStream str_stream;

	auto writeDataFunc = [&str_stream] (const void *data, size_t size) -> size_t
	{
		str_stream.Write(data, size);
		return str_stream.LastWrite();
	};

	url.SetWriteDataCallback(writeDataFunc);

	if(!url.Perform())
	{
		wxLogError(L"Can not fetch specified URL ('%s')", url.GetUrl());
		return wxEmptyString;
	}

	return str_stream.GetString();
}

bool Updater::DownloadFile(const wxString &srcUrl, const wxString &dstPath, const DownloadFunc &callback)
{
	wxFileOutputStream fileStream(dstPath);

	if(!fileStream.IsOk())
	{
		wxLogError(L"Can not open file '%s'", dstPath);
		return false;
	}

	UrlHelper url(srcUrl);

	if(!url.IsOk())
	{
		wxLogError(L"Can not open URL '%s' - %s", url.GetUrl(), url.GetError());
	}

	url.SetWriteDataCallback([&fileStream] (const void *data, size_t size) -> size_t
	{
		fileStream.Write(data, size);
		return fileStream.LastWrite();
	});

	time_t last_time = time(nullptr);

	url.SetProgressCallback([&url, &callback, &last_time] (wxLongLong_t bytes, wxLongLong_t total) -> bool
	{
		if(callback && difftime(time(nullptr), last_time) >= 1.0)
		{
			last_time = time(nullptr);
			return callback(bytes, total, url.GetDownloadSpeed());
		}

		return false;
	});

	if(!url.Perform())
	{
		wxLogError(L"Can not fetch specified URL ('%s')", url.GetUrl());
		return false;
	}

	return true;
}

bool Updater::ExtractFilesFromZip(const wxString &srcPath, const wxString &dstPath, const ExtractFunc &callback)
{
	wxFileInputStream fileStream(srcPath);

	if(!fileStream.IsOk())
	{
		wxLogError(L"Can not open file '%s'", srcPath);
		return false;
	}

	wxMBConvUTF8 mbConv;
	wxZipInputStream zipStream(fileStream, mbConv);

	if(!zipStream.IsOk())
	{
		wxLogError(L"Zip stream is not good '%s'", srcPath);
		return false;
	}

	if(!wxDirExists(dstPath))
	{
		wxLogVerbose(L"Creating path '%s'", dstPath);
		wxMkDir(dstPath, wxS_DIR_DEFAULT);
	}

	bool logPermission = ReadConfigValue(wxConfigBase::Get(), L"/LogPermissions", true);

	size_t cntr = 0;
	size_t total = zipStream.GetTotalEntries();

	wxZipEntry *entry = zipStream.GetNextEntry();

	while(entry)
	{
		wxString entryName = entry->GetName();
		wxString fileName = dstPath + wxFileName::GetPathSeparator() + entryName;

		if(callback(++cntr, total, fileName)) break;

		if(entry->IsDir()) wxFileName::Mkdir(fileName, entry->GetMode(), wxPATH_MKDIR_FULL);
		else
		{
			zipStream.OpenEntry(*entry);

			if(!zipStream.CanRead())
			{
				wxLogError(L"Can not read zip entry '%s'", entryName);
				return false;
			}

			wxString filePath = wxFileName(fileName).GetPath();
			if(!wxDirExists(filePath)) wxMkDir(filePath, wxS_DIR_DEFAULT);

			wxFileOutputStream dstStream(fileName);

			//wxLogMessage(L"Extracting '%s' to '%s'", entryName, fileName);
 
			if(!dstStream.IsOk())
			{
				wxLogError(L"Can not open file '%s'", fileName);

				if(logPermission)
				{
					wxString owner, group;
					std::map<wxString, SimpleAccessRights> permissions;

					GetFilePermissions(fileName, owner, group, permissions);

					wxLogMessage(L"File permissions");
					wxLogMessage(L"Owner: %s", owner);
					//wxLogMessage(L"Group: %s", group);

					for(auto it = permissions.begin(); it != permissions.end(); it++)
						wxLogMessage(L"%s: %s", it->first, it->second.ToShortString());
				}

				return false;
			}

			zipStream.Read(dstStream);
			zipStream.CloseEntry();
		}

		entry = zipStream.GetNextEntry();
	}

	return true;
}

wxString Updater::GetFileSHA1(const wxString &path)
{
	boost::uuids::detail::sha1 sha;

	wxFileInputStream stream(path);

	if(!stream.IsOk())
	{
		wxLogError(L"Can not open file '%s'", path);
		return wxEmptyString;
	}

	std::array<char, 10000> buf;
	size_t readed = 0;

	while(stream.CanRead())
	{
		stream.Read(static_cast<void *>(buf.data()), buf.size());
		sha.process_bytes(buf.data(), stream.LastRead());

		readed += stream.LastRead();
	}

	char hash[20];
	unsigned int digest[5];

	sha.get_digest(digest);

	for(size_t i = 0; i < 5; ++i)
	{
	    hash[4 * i + 0] = (digest[i] >> 24) & 0xff;
	    hash[4 * i + 1] = (digest[i] >> 16) & 0xff;
	    hash[4 * i + 2] = (digest[i] >> 8) & 0xff;
	    hash[4 * i + 3] = digest[i] & 0xff;
	}

	std::stringstream strStream(std::stringstream::out);

	strStream << std::hex;

	for(size_t i = 0; i < 20; i++)
		strStream << ((hash[i] & 0xf0) >> 4) << (hash[i] & 0x0f);

	return wxString(strStream.str());
}

UpdateInfo Updater::ParseUpdateInfoXML(const wxString &src) const
{
	wxStringInputStream stream(src);
	wxXmlDocument doc(stream);
	
	wxXmlNode *node = doc.GetRoot();
	if(!node || node->GetName() != L"update") throw Exception(L"update node not found");
	
	UpdateInfo info;
	
	wxString str;
	long lval;
	
	if(node->GetAttribute(L"format_version", &str) && str.ToLong(&lval))
		info.format_version = static_cast<int>(lval);
	
	wxXmlNode *appNode = node->GetChildren();
	if(!appNode || appNode->GetName() != L"app") throw Exception(L"app node not found");
	
	if(!appNode->GetAttribute(L"name", &info.app_name)) throw Exception(L"name attribute not found");
	if(!appNode->GetAttribute(L"version", &info.app_version)) throw Exception(L"version attribute not found");
	if(!appNode->GetAttribute(L"process_name", &info.process_name)) throw Exception(L"process_name attribute not found");

	wxXmlNode *infoNode = GetChildren(L"info", appNode);
	if(infoNode) info.info = infoNode->GetNodeContent();
	
	wxXmlNode *elementsNode = GetChildren(L"elements", appNode);
	if(!elementsNode || elementsNode->GetName() != L"elements") throw Exception(L"elements node not found");
	
	wxXmlNode *elementNode = elementsNode->GetChildren();
	
	while(elementNode && elementNode->GetName() == L"element")
	{
		info.elements.push_back(ParseElementNode(elementNode));
		
		elementNode = elementNode->GetNext();
	}
	
	return info;
}

wxXmlNode *Updater::GetChildren(const wxString &name, wxXmlNode *parent)
{
	if(!parent) return nullptr;

	wxXmlNode *node = parent->GetChildren();

	while(node && node->GetName() != name)
		node = node->GetNext();

	return node;
}

std::shared_ptr<UpdateElement> Updater::ParseElementNode(wxXmlNode *elementNode) const
{
	std::shared_ptr<UpdateElement> element(new UpdateElement);
	elementNode->GetAttribute(L"type", &element->type);
	
	wxXmlNode *node = elementNode->GetChildren();
	
	while(node)
	{
		if(node->GetName() == L"file")
		{
			UpdateFile update_file;
			
			node->GetAttribute(L"src", &update_file.src);
			node->GetAttribute(L"hash", &update_file.hash);

			wxString str_size;
			node->GetAttribute(L"size", &str_size);

			if(!str_size.ToULongLong(&update_file.size)) update_file.size = 0;
			
			element->files.push_back(update_file);
		}
		
		node = node->GetNext();
	}
	
	return element;
}

bool Updater::Check(UpdateInfo *info)
{
	wxString upd_xml = GetUpdateInfo(L"/update_info.xml");

	if(upd_xml.IsEmpty())
	{
		wxLogWarning(L"update_info.xml is empty");
		return false;
	}
	if(info)
	{
		try
		{
			*info = ParseUpdateInfoXML(upd_xml);
		}
		catch(Exception &e)
		{
			wxLogWarning(e.GetDescription());
			return false;
		}
	}

	return true;
}

void Updater::SetProgressCallback(const ProgressFunc &func)
{
	progressFunc = func;
}