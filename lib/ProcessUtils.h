#pragma once

#include <wx/string.h>
#include <vector>

struct ProcessInfo
{
	long pid;
	wxString name;
};

class ProcessUtils
{
public:
	static void GetProcessList(std::vector<ProcessInfo> &proc_list);
	static bool CheckProcess(const wxString &name);
};