#include "ProcessUtils.h"
#include <iostream>
#include <algorithm>

#ifdef __WXMSW__

#include <Windows.h>
#include <TlHelp32.h>

#define wxLOG_COMPONENT "updater"

void ProcessUtils::GetProcessList(std::vector<ProcessInfo> &proc_list)
{
	proc_list.clear();

	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if(hSnapshot == INVALID_HANDLE_VALUE)
	{
		std::cerr << "invalid toolhelp snapshot handle" << std::endl;
		return;
	}

	PROCESSENTRY32 pe;
	pe.dwSize = sizeof(PROCESSENTRY32);

	if(!Process32First(hSnapshot, &pe))
	{
		std::cerr << "Process32First failed" << std::endl;
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

bool ProcessUtils::CheckProcess(const wxString &name)
{
	std::vector<ProcessInfo> proc_list;
	GetProcessList(proc_list);

	return std::find_if(proc_list.begin(), proc_list.end(), [&name] (const ProcessInfo& info) {return info.name == name;}) != proc_list.end();
}

#endif