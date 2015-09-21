#include "Utils.h"

#include <wx/log.h>
#include <memory>

#define wxLOG_COMPONENT "updater"

#ifdef __WXMSW__

#define _WIN32_WINNT 0x0501 // hm..
#include <windows.h>
#include <lm.h>

std::pair<wxString, wxString> LookupSid(PSID pSid)
{
	std::pair<wxString, wxString> result;

	std::unique_ptr<wchar_t> accountName;
	DWORD accountNameLen = 0;
	std::unique_ptr<wchar_t> domainName;
	DWORD domainNameLen = 0;
	SID_NAME_USE sidNameUse = SidTypeUnknown;

	if(!LookupAccountSid(0, pSid, 0, &accountNameLen, 0, &domainNameLen, &sidNameUse) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		accountName.reset(new wchar_t[accountNameLen]);
		domainName.reset(new wchar_t[domainNameLen]);

		if(LookupAccountSid(0, pSid, accountName.get(), &accountNameLen, domainName.get(), &domainNameLen, &sidNameUse))
		{
			result.first = wxString(accountName.get(), accountNameLen);
			result.second = wxString(domainName.get(), domainNameLen);
		}
	}

	return result;	
}

wxString CreateUserName(const std::pair<wxString, wxString> &user)
{
	if(user.second.IsEmpty()) return user.first;
	else return user.second + L"\\" + user.first;
}

bool GetFilePermissions(const wxString &filename, wxString &owner, wxString &group, std::map<wxString, SimpleAccessRights> &permissions)
{
	DWORD info = OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
	std::unique_ptr<SECURITY_DESCRIPTOR> desc;
	DWORD size = 0;

	PSID pSid = NULL;
	BOOL defaulted = FALSE;
	PACL pDacl = NULL;
	BOOL daclPresent = FALSE;

	permissions.clear();

	if(!GetFileSecurity(filename.c_str(), info, 0, 0, &size) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		wxLogError(L"GetFileSecurity first failed with %d code", GetLastError());
		return false;
	}

	desc.reset(static_cast<SECURITY_DESCRIPTOR *>(malloc(size)));
	if(!desc)
	{
		wxLogError(L"Out of memory for security descriptor");
		return false;
	}

	if(!GetFileSecurity(filename.c_str(), info, desc.get(), size, &size))
	{
		wxLogError(L"GetFileSecurity second failed with %d code", GetLastError());
		return false;
	}

	if(GetSecurityDescriptorOwner(desc.get(), &pSid, &defaulted))
	{
		if(pSid) owner = LookupSid(pSid).first;
		else wxLogWarning(L"No owner found");
	}
	else wxLogWarning(L"GetSecurityDescriptorOwner failed");

	pSid = NULL;

	if(GetSecurityDescriptorGroup(desc.get(), &pSid, &defaulted))
	{
		if(pSid) group = LookupSid(pSid).first;
		else wxLogWarning(L"No group found");
	}
	else wxLogWarning(L"GetSecurityDescriptorOwner failed");

	if(GetSecurityDescriptorDacl(desc.get(), &daclPresent, &pDacl, &defaulted))
	{
		if(daclPresent == FALSE || pDacl == NULL) wxLogWarning(L"No DACL was found (all access is denied), or a NULL DACL (unrestricted access) was found");
		else
		{
			ACL_SIZE_INFORMATION aclSizeInfo;

			if(GetAclInformation(pDacl, &aclSizeInfo, sizeof(aclSizeInfo), AclSizeInformation))
			{
				ACCESS_ALLOWED_ACE *pAce = NULL;

				for(DWORD i = 0; i < aclSizeInfo.AceCount; i++)
				{
					if(!GetAce(pDacl, i, (LPVOID *)&pAce))
					{
						wxLogWarning(L"GetAce failed");
						continue;
					}

					SimpleAccessRights sar;

					PSID sid = static_cast<PSID>(&pAce->SidStart);

					if(pAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
					{
						sar.allowedRead = pAce->Mask & FILE_READ_DATA;
						sar.allowedWrite = pAce->Mask & FILE_WRITE_DATA;
						sar.allowedModify = pAce->Mask & FILE_APPEND_DATA;
						sar.allowedExecute = pAce->Mask & FILE_EXECUTE;
					}
					else if(pAce->Header.AceType == ACCESS_DENIED_ACE_TYPE) 
					{
						sar.deniedRead = pAce->Mask & FILE_READ_DATA;
						sar.deniedWrite = pAce->Mask & FILE_WRITE_DATA;
						sar.deniedModify = pAce->Mask & FILE_APPEND_DATA;
						sar.deniedExecute = pAce->Mask & FILE_EXECUTE;				
					}

					wxString user = CreateUserName(LookupSid(sid));
					permissions[user] = sar;
				}
			}
		}
	}
	else wxLogWarning(L"GetSecurityDescriptorDacl failed");

	return true;
}

PSID GetCurrentProcessOwnerSid()
{
	HANDLE hToken = NULL;
	std::unique_ptr<TOKEN_USER> tokenUser;
	std::unique_ptr<TOKEN_GROUPS> tokenGroups;
	DWORD size = 0;
	PSID pSid = NULL;

	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE | TOKEN_QUERY | STANDARD_RIGHTS_READ, &hToken))
	{
		wxLogError(L"OpenProcessToken failed with %d code", GetLastError());
		return pSid;
	}

	// Getting token user information

	if(!GetTokenInformation(hToken, TokenUser, NULL, size, &size) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		wxLogError(L"GetTokenInformation first failed with %d code", GetLastError());

		CloseHandle(hToken);
		return pSid;	
	}

	tokenUser.reset(static_cast<TOKEN_USER *>(malloc(size)));
	if(!tokenUser)
	{
		wxLogError(L"Out of memory for token user");

		CloseHandle(hToken);
		return pSid;
	}

	if(!GetTokenInformation(hToken, TokenUser, tokenUser.get(), size, &size))
	{
		wxLogError(L"GetTokenInformation second failed with %d code", GetLastError());

		CloseHandle(hToken);
		return pSid;
	}

	pSid = tokenUser->User.Sid;

	CloseHandle(hToken);
	return pSid;
}

void GetUserGroupsSid(PSID pUserSid, std::vector<PSID> &pGroupsSid)
{
	HANDLE hToken = NULL;
	std::unique_ptr<TOKEN_GROUPS> tokenGroups;
	DWORD size = 0;

	pGroupsSid.clear();

	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_IMPERSONATE | TOKEN_QUERY | STANDARD_RIGHTS_READ, &hToken))
	{
		wxLogError(L"OpenProcessToken failed with %d code", GetLastError());
		return;
	}

	// Getting token groups information

	if(!GetTokenInformation(hToken, TokenGroups, NULL, size, &size) && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
		wxLogError(L"GetTokenInformation first failed with %d code", GetLastError());

		CloseHandle(hToken);
		return;
	}

	tokenGroups.reset(static_cast<TOKEN_GROUPS *>(malloc(size)));
	if(!tokenGroups)
	{
		wxLogError(L"Out of memory for token groups");

		CloseHandle(hToken);
		return;
	}

	if(!GetTokenInformation(hToken, TokenGroups, tokenGroups.get(), size, &size))
	{
		wxLogError(L"GetTokenInformation second failed with %d code", GetLastError());

		CloseHandle(hToken);
		return;
	}

	std::pair<wxString, wxString> user = LookupSid(pUserSid);

	for(DWORD i = 0; i < tokenGroups->GroupCount; i++)
	{
		std::pair<wxString, wxString> group = LookupSid(tokenGroups->Groups[i].Sid);

		if(user.second == group.second)
			pGroupsSid.push_back(tokenGroups->Groups[i].Sid);
	}
}

wxString GetCurrentProcessOwner()
{
	PSID pSid = GetCurrentProcessOwnerSid();

	if(pSid) return CreateUserName(LookupSid(pSid));
	else return wxEmptyString;
}

std::vector<wxString> GetCurrentProccessOwnerGroups()
{
	std::vector<wxString> groups;

	PSID pUserSid = GetCurrentProcessOwnerSid();

	if(pUserSid)
	{
		std::pair<wxString, wxString> user = LookupSid(pUserSid);
		wxString strUser = CreateUserName(user);

		LPLOCALGROUP_USERS_INFO_0 buf = NULL;
		DWORD entriesRead = 0;
		DWORD totalEntries = 0;

		NET_API_STATUS nStatus = NetUserGetLocalGroups(0, strUser.c_str(), 0, LG_INCLUDE_INDIRECT, (LPBYTE *) &buf, MAX_PREFERRED_LENGTH, &entriesRead, &totalEntries);

		if(nStatus == NERR_Success)
		{
			for(DWORD i = 0; i < entriesRead; i++)
			{
				LPLOCALGROUP_USERS_INFO_0 info = buf + i;
				groups.push_back(info->lgrui0_name);
			}
		}
		else wxLogError(L"NetUserGetLocalGroups failed with %d status", nStatus);

		if(buf) NetApiBufferFree(buf);
	}

	return groups;
}

bool CurrentProcessOwnerAdmin()
{
	BOOL is_admin = false;
	SID_IDENTIFIER_AUTHORITY NtAuthority = {SECURITY_NT_AUTHORITY};
	PSID pAdminSid;

	// check for administrator rights

	if(AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdminSid))
	{
		if(!CheckTokenMembership(NULL, pAdminSid, &is_admin))
			is_admin = FALSE;

		FreeSid(pAdminSid); 
	}
	else wxLogError(L"AllocateAndInitializeSid failed with %d code", GetLastError());

	return is_admin;
}

#else

wxString GetCurrentProcessOwner()
{
	return wxEmptyString;
}

std::vector<wxString> GetCurrentProccessOwnerGroups()
{
	return std::vector<wxString>();
}

bool GetFilePermissions(const wxString &filename, wxString &owner, wxString &group, std::map<wxString, SimpleAccessRights> &permissions)
{
	return false;
}

#endif

SimpleAccessRights::SimpleAccessRights() : allowedRead(false), allowedWrite(false), allowedModify(false),
	deniedRead(false), deniedWrite(false), deniedModify(false)
{

}

wxString SimpleAccessRights::ToShortString() const
{
	wxString str;

	bool read = allowedRead & ~deniedRead;
	bool write = allowedWrite & ~deniedWrite;
	bool execute = allowedExecute & ~deniedExecute;

	str += read ? L"r" : L"-";
	str += write ? L"w" : L"-";
	str += execute ? L"x" : L"-";

	return str;
}
