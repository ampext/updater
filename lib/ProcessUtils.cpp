#include "ProcessUtils.h"
#include <wx/log.h>
#include <algorithm>

#define wxLOG_COMPONENT "updater"

#ifdef __WXMSW__

#include <Windows.h>
#include <TlHelp32.h>

void ProcessUtils::GetProcessList(std::vector<ProcessInfo> &proc_list)
{
	proc_list.clear();

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(hSnapshot == INVALID_HANDLE_VALUE)
	{
		wxLogError(L"Invalid toolhelp snapshot handle");
		return;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);

	if(!Process32First(hSnapshot, &pe))
	{
		wxLogError(L"Process32First failed");
		CloseHandle(hSnapshot);
		return;
	}

	do
	{
		ProcessInfo info;
		info.pid = pe.th32ProcessID;
		info.name = wxString(pe.szExeFile);

		proc_list.push_back(info);
	}
	while(Process32Next(hSnapshot, &pe));

	CloseHandle(hSnapshot);
}

bool ProcessUtils::FindProcess(const wxString &name, ProcessInfo *info)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(hSnapshot == INVALID_HANDLE_VALUE)
	{
		wxLogError(L"Invalid toolhelp snapshot handle");
		return false;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);

	if(!Process32First(hSnapshot, &pe))
	{
		wxLogError(L"Process32First failed");
		CloseHandle(hSnapshot);
		return false;
	}

	do
	{
		if(name == pe.szExeFile)
		{
			if(info)
			{
				info->pid = pe.th32ProcessID;
				info->name = wxString(pe.szExeFile);
			}

			CloseHandle(hSnapshot);
			return true;
		}
	}
	while(Process32Next(hSnapshot, &pe));

	CloseHandle(hSnapshot);
	return false;
}

bool ProcessUtils::CheckProcess(const wxString &name)
{
	return FindProcess(name);
}

bool ProcessUtils::KillProcess(const wxString &name)
{
	wxLogMessage(L"Trying to terminate process %s", name);

	ProcessInfo info;
	if(!FindProcess(name, &info)) return false;

	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, 0, info.pid);
	if(hProcess != NULL)
	{
		BOOL res = TerminateProcess(hProcess, 1);
		CloseHandle(hProcess);

		if(res)
		{
			wxLogMessage(L"Process terminated");
			return true;
		}
	}

	wxLogWarning(L"Can not open process with pid %d for termination (error = %d)", info.pid, GetLastError());
	return false;
}

#else

void ProcessUtils::GetProcessList(std::vector<ProcessInfo> &proc_list)
{
	proc_list.clear();
}

bool ProcessUtils::FindProcess(const wxString &name, ProcessInfo *info = nullptr)
{
	return false;
}

bool ProcessUtils::CheckProcess(const wxString &name)
{
	return false;	
}

bool ProcessUtils::KillProcess(const wxString &name)
{
	return false;
}

#endif
