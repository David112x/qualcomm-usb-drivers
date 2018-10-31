// QiKInstaller.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <sstream>
#include <VersionHelpers.h>

using namespace std;

typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

LPFN_ISWOW64PROCESS fnIsWow64Process;

BOOL IsWow64()
{
	BOOL bIsWow64 = FALSE;

	//IsWow64Process is not available on all supported versions of Windows.
	//Use GetModuleHandle to get a handle to the DLL that contains the function
	//and GetProcAddress to get a pointer to the function if available.

	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(
		GetModuleHandle(TEXT("kernel32")), "IsWow64Process");

	if (NULL != fnIsWow64Process)
	{
		if (!fnIsWow64Process(GetCurrentProcess(), &bIsWow64))
		{
			//handle error
	}
}
	return bIsWow64;
}

DWORD RunProcess(std::string processName)
{
	STARTUPINFO si;
	::ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);

	PROCESS_INFORMATION pi;
	::ZeroMemory(&pi, sizeof(pi));
	
	std::wstring stemp = std::wstring(processName.begin(), processName.end());
	LPCWSTR sw = stemp.c_str();

	// Start the child process
	DWORD exitCode;
	BOOL bProc = ::CreateProcess(0, (LPWSTR)sw, 0, 0, FALSE, 0, 0, 0, &si, &pi);
	printf("value of bProc %d\n", bProc);
	if (bProc == FALSE)
	{
		DWORD ec = ::GetLastError();
		printf("last error code %d\n", ec);		
		return (DWORD)-1;
	}

	// Wait until child process exits and store exit code
	::WaitForSingleObject(pi.hProcess, INFINITE);
	::GetExitCodeProcess(pi.hProcess, &exitCode);
	printf("exit code %d\n", exitCode);

	// Close process and thread handles 
	::CloseHandle(pi.hProcess);
	::CloseHandle(pi.hThread);
	
	return exitCode;
}


int main(int argc, char** argv)
{
	char installFlag = ' ';
	int iUseLegacyDriver = 0;
	bool bUseLegacyDriver = false;
	string mainPath = "";
	if (argc > 1)
	{
		mainPath = argv[1];
		printf("mainPath = %s\n", mainPath);
		if (argc > 2)
		{
			installFlag = *argv[2];
			printf("install flag %c\n", installFlag);
			if (installFlag == 'i')
			{
				if (argc >= 4)
				{
					istringstream ss(argv[3]);
					if (!(ss >> iUseLegacyDriver))
					{
						printf("invalid number %s\n", argv[3]);
					}
					if (iUseLegacyDriver == 1)
					{
						bUseLegacyDriver = true;
					}
					printf("use ethernet driver %d\n", bUseLegacyDriver);
				}
			}
		}						
	}
	

	bool bIsWin7OrLater = IsWindows7OrGreater();
	bool bIsWin10 = IsWindows10OrGreater();
	BOOL bIs64 = IsWow64();
	printf("is win 7 or later? %d\n", bIsWin7OrLater);
	printf("is win 10? %d\n", bIsWin10);	
	
	string installer64 = mainPath + "\\DriverInstaller64.exe";
	string installer32 = mainPath + "\\DriverInstaller32.exe";
	string driversPath = mainPath + "\\fre\\";
	string cmd = "";
	
	if (bIs64 == true)
	{
		_tprintf(TEXT("The process is running under WOW64.\n"));
		if (bIsWin10 == true)
		{
			driversPath.append("\\Windows10\\");
		}
		else
		{
			driversPath.append("\\Windows7\\");
		}
		printf("installer64:%s\n", installer64);
		cmd = installer64;
	}
	else
	{
		_tprintf(TEXT("The process is not running under WOW64.\n"));
		printf("installer32:%s\n", installer32);
		if (bIsWin10 == true)
		{
			driversPath.append("\\Windows10\\");
		}
		else
		{
			driversPath.append("\\Windows7\\");
		}
		cmd = installer32;
	}
	cmd += " ";	
	
	printf("driversPath:%s\n", driversPath);		
	
	if (bIsWin7OrLater == false)
	{
		if (installFlag == 'i')
		{
			if (bUseLegacyDriver == false)
			{
				cmd += "\"/i|0|";
			}
			else
			{
				cmd += "\"/i|1|";
			}
		}
		else
		{
			cmd += "\"/x|1|";
		}		
	}
	else
	{
		if (installFlag == 'i')
		{
			if (bUseLegacyDriver == false)
			{
				cmd += "\"/I|0|";
			}
			else
			{
				cmd += "\"/I|1|";
			}
		}
		else
		{
			cmd += "\"/X|1|";
		}		
	}	
	
	cmd += driversPath;	
	printf("CMD:%s\n", cmd);
	DWORD exitCode = RunProcess(cmd);
	if (exitCode == 0)
	{
		printf("Installation successful!!\n");
	}
	else
	{
		printf("Installation unsuccessful!!\n");
	}	
	
    return 0;
}