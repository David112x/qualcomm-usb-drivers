/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                           I N F D E V. C

GENERAL DESCRIPTION
  Scan and remove USB VID_06C5 related INF files from system

  Copyright (c) 2015 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#include "Stdafx.h"
#include "infdev.h"

// Global Variables
static PUCHAR gFileDataRaw;
static UINT gNumTotal = 0;
static gChangeDir = FALSE;
INT gExecutionMode = EXEC_MODE_PREVIEW;

VOID DisplayTime(PCHAR Title, BOOL NewLine)
{
   SYSTEMTIME lt;

   GetLocalTime(&lt);
   printf("[%02d:%02d:%02d.%03d] %s (%d files) WOW64-%d\n",
           lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds, Title,
           gNumTotal, RunningWOW64());
   if (NewLine == TRUE)
   {
      printf("\n");
   }
} // DisplayTime

INT MainRemovalTask(VOID)
{
   //if (CheckOSCompatibility() == FALSE)
   //{
   //   return 1;
   //}

   // MessageBox(NULL, TEXT("MAIN_REMOVAL_TASK"),NULL,MB_YESNO);
   DisplayTime("=== Start of Scanning ===", FALSE);
   if (gExecutionMode == EXEC_MODE_PREVIEW)
   {
      printf("   Mode: Preview\n\n");
   }
   else
   {
      printf("   Mode: Removal\n\n");
	  // MessageBox(NULL, TEXT("OEM"),NULL,MB_YESNO);
   }

   gFileDataRaw = (PUCHAR)malloc(INF_SIZE_MAX);
   if (gFileDataRaw == NULL)
   {
      printf("error: no memory for gFileDataRaw\n");
      return 1;
   }

   // MessageBox(NULL, TEXT(MATCH_VID),NULL,MB_YESNO);

   // 1. remove devnode
   ScanAndRemoveDevice(TEXT(MATCH_VID));
   // 2. remove INF
   ScanInfFiles(QC_INF_PATH);
   // 3. reset driver config
   ResetDriverConfig();

   free(gFileDataRaw);

   // PackageRemoval(TEXT("Qualcomm USB Drivers For Windows"));

   DisplayTime("=== End of Scanning ===", TRUE);

   return 0;
}  // MainRemovalTask

BOOL CheckOSCompatibility(VOID)
{
   BOOL builtForWin64 = FALSE;
   BOOL bCompatible = TRUE;
   BOOL wow64;

   #ifdef _WIN64
   builtForWin64 = TRUE;
   #endif

   #ifdef AMD64
   builtForWin64 = TRUE;
   #endif

   wow64 = RunningWOW64();

   // printf("TRACE: builtForWin64 (%d) wow64(%d)\n", builtForWin64, wow64);

   if ((builtForWin64 == TRUE) && (wow64 == FALSE))
   {
      printf("   This program runs on 64-bit Windows instead of 32-bit Windows.\n");
      bCompatible = FALSE;
   }
   if ((builtForWin64 == FALSE) && (wow64 == TRUE))
   {
      printf("   This program runs on 32-bit Windows instead of 64-bit Windows.\n");
      bCompatible = FALSE;
   }

   return bCompatible;

}  // CheckCompatibility

VOID ScanInfFiles(LPCTSTR InfPath)
{
   WIN32_FIND_DATA fileData;
   HANDLE          fileHandle;
   BOOL            notDone = TRUE;
   UINT            idx = 0;
   WCHAR           fullPath[MAX_PATH];
   WCHAR           searchPath[MAX_PATH];
   WCHAR           currentDir[MAX_PATH];
   BOOL            dirSet = FALSE;
   LARGE_INTEGER   infSize;
   ULONG           dataSize;

   if (gChangeDir == TRUE)
   {
      // save current dir
      GetCurrentDirectory(MAX_PATH, (LPTSTR)currentDir);

      if (FALSE == SetCurrentDirectory(InfPath))
      {
         StringCchCopy(searchPath, MAX_PATH, InfPath);
         StringCchCat(searchPath, MAX_PATH, TEXT(OEM_INF));
      }
      else
      {
         StringCchCopy(searchPath, MAX_PATH, TEXT("."));
         StringCchCat(searchPath, MAX_PATH, TEXT(OEM_INF));
         dirSet = TRUE;
      }
   }
   else
   {
      StringCchCopy(searchPath, MAX_PATH, InfPath);
      StringCchCat(searchPath, MAX_PATH, TEXT(OEM_INF));
   }

   printf("\n   INF Scan Path: <%ws>\n", InfPath);

   fileHandle = FindFirstFile(searchPath, &fileData);
   if (fileHandle == INVALID_HANDLE_VALUE)
   {
      printf("FindFirstFile failure\n");
      return;
   }

   while (notDone == TRUE)
   {
      if ((fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
      {
         // got a file
         idx++;
         if (dirSet == FALSE)
         {
            StringCchCopy(fullPath, MAX_PATH, InfPath);
            StringCchCat(fullPath, MAX_PATH, TEXT("\\"));
         }
         else
         {
            StringCchCopy(fullPath, MAX_PATH, TEXT(".\\"));
         }
         infSize.LowPart = fileData.nFileSizeLow;
         infSize.HighPart = fileData.nFileSizeHigh;
         StringCchCat(fullPath, MAX_PATH, fileData.cFileName);
         // printf("   full path <%ws>\n", fullPath);
         if (infSize.QuadPart > (INF_SIZE_MAX - 2))
         {
            MatchingInf(fullPath, fileData.cFileName, (INF_SIZE_MAX-2));
         }
         else
         {
            dataSize = (ULONG)infSize.QuadPart;
            MatchingInf(fullPath, fileData.cFileName, dataSize);
         }
      }
      notDone = FindNextFile(fileHandle, &fileData);
   }  // while 

   FindClose(fileHandle);

   if (gChangeDir == TRUE)
   {
      SetCurrentDirectory((LPTSTR)currentDir);
   }

}  // ScanInfFiles

BOOL MatchingInf(PCTSTR InfFullPath, PCTSTR InfFileName, ULONG DataSize)
{
   UINT errLine = 0;
   HINF myHandle;
   INFCONTEXT infCtxt;
   BOOL bResult = TRUE, bResult1;
   DWORD requiredSize = 0;
   UINT finalSection = 0;
   UINT lineCount = 0;
   BOOL matchFound = FALSE;
   DWORD bytesRead;
   BOOL notAscii = TRUE;
   HRESULT res;
   PCHAR matchType = "A";

   gNumTotal++;
   // printf("Processing %ws\n", InfFullPath);

   myHandle = CreateFile
              (
                 InfFullPath,
                 GENERIC_READ,
                 FILE_SHARE_READ,
                 NULL,
                 OPEN_EXISTING,
                 FILE_ATTRIBUTE_NORMAL,
                 NULL
              );
   if (myHandle == INVALID_HANDLE_VALUE)
   {
      printf("Error: SetupOpenInfFile <%ws> 0x%X\n", InfFullPath, GetLastError());
      return FALSE;
   }

   ZeroMemory(gFileDataRaw, INF_SIZE_MAX);

   bResult = ReadFile
             (
                myHandle,
                (PVOID)gFileDataRaw,
                DataSize,
                &bytesRead,
                NULL
             );
   if (bResult == TRUE)
   {
      // printf("Read <%ws> %u bytes\n", InfFullPath, bytesRead, DataSize);
   }
   else
   {
      printf("Error: Failed to read <%ws>\n", InfFullPath);
      return FALSE;
   }

   if (bytesRead < 8)
   {
      // printf("Error: <%ws> insufficient data\n", InfFullPath);
   }

   gFileDataRaw[bytesRead+1] = 0;
   gFileDataRaw[bytesRead+2] = 0;

   // simple detection of encoding format
   if (gFileDataRaw[0] == 0xEF || gFileDataRaw[0] == 0xFE || gFileDataRaw[0] == 0xFF ||
       gFileDataRaw[0] == 0x00)
   {
      notAscii = TRUE;
   }
   if (gFileDataRaw[0] < 0x80 && gFileDataRaw[1] < 0x80)
   {
      notAscii = FALSE;
   }

   // printf("[%ws] (%uB) 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X NotASCII %d\n", InfFullPath, bytesRead,
   //         gFileDataRaw[0], gFileDataRaw[1], gFileDataRaw[2], gFileDataRaw[3], gFileDataRaw[4], notAscii);

   // Filter based on VID/PID to avoid removal of commercial GOBI devices
   // For more aggressive removal based on VID only, comment out the PID lines
   if (notAscii == FALSE)
   {
      // traditional ASCII file
      if (strstr((char *)gFileDataRaw, (char *)MATCH_VID))
      {
         if ((strstr((char *)gFileDataRaw, (char *)MATCH_PID) != NULL) ||
             (strstr((char *)gFileDataRaw, (char *)MATCH_PID2) != NULL) ||
             (strstr((char *)gFileDataRaw, (char *)MATCH_PID3) != NULL))
         {
            matchType = "A";
            matchFound = TRUE;
         }
         else
         {
            printf("(A) Candidate but not to be removed: <%ws>\n", InfFullPath);
         }
      }
         
   }
   else
   {
      if (StrStrW((PTSTR)gFileDataRaw, TEXT(MATCH_VID)) != NULL)
      {
         if ((StrStrW((PTSTR)gFileDataRaw, TEXT(MATCH_PID)) != NULL) ||
             (StrStrW((PTSTR)gFileDataRaw, TEXT(MATCH_PID2)) != NULL) ||
             (StrStrW((PTSTR)gFileDataRaw, TEXT(MATCH_PID3)) != NULL))
         {
            matchType = "W";
            matchFound = TRUE;
         }
         else
         {
            printf("(W) Candidate but not to be removed: <%ws>\n", InfFullPath);
         }
      }
   }

   CloseHandle(myHandle);

   // Confirm INF candidate
   if (matchFound == TRUE)
   {
      matchFound = ConfirmInfFile(InfFullPath);
   }

   if (matchFound == TRUE)
   {
      InfRemoval(InfFullPath, InfFileName, matchType);
   }

   return matchFound;
}  // MatchingInf

BOOL ConfirmInfFile(PCTSTR InfFullPath)
{
   UINT errLine = 0;
   HINF myHandle;
   INFCONTEXT infCtxt;
   BOOL bResult = TRUE, bResult1;
   WCHAR lineText[LINE_LEN_MAX];
   DWORD requiredSize = 0;
   UINT finalSection = 0;
   UINT lineCount = 0;
   BOOL matchConfirmed = FALSE;

   myHandle = SetupOpenInfFile(InfFullPath, NULL, INF_STYLE_WIN4, &errLine);
   if (myHandle == INVALID_HANDLE_VALUE)
   {
      printf("Error: SetupOpenInfFile <%ws> 0x%X\n", InfFullPath, GetLastError());
      return FALSE;
   }

   bResult = SetupFindFirstLine(myHandle, TEXT("Version"), TEXT("Class"), &infCtxt);
   if (bResult == FALSE)
   {
      printf("Error: SetFindFirstLine <%ws> 0x%X\n", InfFullPath, GetLastError());
      SetupCloseInfFile(myHandle);
      return FALSE;
   }

   bResult = SetupGetLineText(&infCtxt, NULL, NULL, NULL, lineText, LINE_LEN_MAX, &requiredSize);
   if (bResult == TRUE)
   {
      // printf("<%ws> Line[%02d, %04d] <%ws> %dB\n", InfFullPath, infCtxt.Section,
      //         infCtxt.Line, lineText, requiredSize);
      if (DeviceMatch(lineText, requiredSize) == TRUE)
      {
         // printf("Confirmed <%ws>\n", InfFullPath);
         matchConfirmed = TRUE;
      }
   }
   else
   {
      printf("Error: SetupGetLineText <%ws> 0x%X\n", InfFullPath, GetLastError());
   }

   SetupCloseInfFile(myHandle);

   return matchConfirmed;

}  // ConfirmInfFile

BOOL DeviceMatch(PTSTR InfText, DWORD TextSize)
{
   CHAR infText[LINE_LEN_MAX];
   DWORD actualSize = sizeof(WCHAR) * TextSize;
   HRESULT res;
   BOOL matchFound = FALSE;

   if (actualSize > LINE_LEN_MAX)
   {
      printf("DeviceMatch: line too long (%uB/%uB)\n", TextSize, LINE_LEN_MAX);
      return FALSE;
   }

   if (StrStrW(InfText, TEXT("Modem")) != NULL)      // MODEM
   {
      matchFound = TRUE;
   }
   else if (StrStrW(InfText, TEXT("Net")) != NULL)   // NET ADAPTR
   {
      matchFound = TRUE;
   }
   else if (StrStrW(InfText, TEXT("Ports")) != NULL) // SER PORT
   {
      matchFound = TRUE;
   }
   else if (StrStrW(InfText, TEXT("USB")) != NULL)   // QDSS, DPL, FILTER
   {
      matchFound = TRUE;
   }
   else if (StrStrW(InfText, TEXT("YunOSUsbDeviceClass")) != NULL)   // YunOS
   {
      matchFound = TRUE;
   }
   else if (StrStrW(InfText, TEXT("AndroidUsbDeviceClass")) != NULL)   // YunOS
   {
      matchFound = TRUE;
   }

   return matchFound;
}  // DeviceMatch

VOID InfRemoval(LPCTSTR FilePath, PCTSTR InfFileName, PCHAR Type)
{
   WCHAR cmdLineW[MAX_PATH];
   WCHAR infName[MAX_PATH], pnfName[MAX_PATH];
   PROCESS_INFORMATION processInfo;
   STARTUPINFO startupInfo;
   BOOL rtnVal;
   DWORD exitCode;
   HRESULT res;
   PCHAR p;
   BOOL bInfRemoved = FALSE;

   if (IsOSVersionVistaOrHigher() == TRUE)
   {
      StringCchCopy(cmdLineW, MAX_PATH, TEXT("C:\\Windows\\system32\\pnputil.exe -f -d "));
      StringCchCat(cmdLineW, MAX_PATH, InfFileName);
   }
   else
   {
      // Need to formulate comand such as "del oem123.*"

      StringCchCopy(cmdLineW, MAX_PATH, TEXT("del "));

      CharLower((PTSTR)FilePath);

      // Change FilePath suffix from inf to *
      p = (PCHAR)StrStr(FilePath, TEXT(".inf"));

      if (p != NULL)
      {
         *p = 0;
         *(p+1) = 0;
         StringCchCat(cmdLineW, MAX_PATH, FilePath);
         StringCchCat(cmdLineW, MAX_PATH, TEXT(".*"));

         StringCchCopy(infName, MAX_PATH, FilePath);
         StringCchCat(infName, MAX_PATH, TEXT(".inf"));
         StringCchCopy(pnfName, MAX_PATH, FilePath);
         StringCchCat(pnfName, MAX_PATH, TEXT(".PNF"));

         if (gExecutionMode != EXEC_MODE_PREVIEW)
         {
            printf("Deleting <%ws>...\n", infName);
            bInfRemoved = DeleteFile(infName);
            if (bInfRemoved == TRUE)
            {
               printf("Deleting <%ws>...\n", pnfName);
               DeleteFile(pnfName);
            }
            else
            {
               printf("   Deletion failure 0x%X\n", GetLastError());
            }
         }
      }
      else
      {
         printf("Error: failed to change file suffix\n");
         StringCchCat(cmdLineW, MAX_PATH, FilePath);
      }
   }

   if (gExecutionMode == EXEC_MODE_PREVIEW)
   {
      printf("(%s) %ws\n", Type, cmdLineW);
   }
   else if (bInfRemoved == FALSE)
   {
      printf("\nDeleting %ws ...\n", InfFileName);

      memset(&processInfo, 0, sizeof(processInfo));
      memset(&startupInfo, 0, sizeof(startupInfo));
      startupInfo.cb = sizeof(startupInfo);

      rtnVal = CreateProcess
               (
                  NULL, // L"C:\\Windows\\pnputil.exe",
                  cmdLineW,
                  NULL,
                  NULL,
                  FALSE,
                  NORMAL_PRIORITY_CLASS,
                  NULL,
                  NULL,
                  &startupInfo,
                  &processInfo
               );

      if (rtnVal == FALSE)
      {
         printf("Error: CreateProcess failure 0x%X\n", GetLastError());
      }

      WaitForSingleObject(processInfo.hProcess, INFINITE);
      GetExitCodeProcess(processInfo.hProcess, &exitCode);
      CloseHandle(processInfo.hProcess);
      CloseHandle(processInfo.hThread);
   }

   return;
}  // InfRemoval

BOOL IsOSVersionVistaOrHigher(VOID)
{
   OSVERSIONINFO version;
   BOOL bResult;

   ZeroMemory(&version, sizeof(OSVERSIONINFO));
   version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   bResult = GetVersionEx(&version);

   if (version.dwMajorVersion >= 6)
   {
      return TRUE;
   }
   return FALSE;

}  // IsOSVersionVistaOrHigher

BOOL RunningWOW64(VOID)
{
   HMODULE moduleHandle;
   WOW64PROCESS_FUNC IsWow64ProcessFunc;
   BOOL bRunningIn64Bit = FALSE;
   HANDLE myProc = GetCurrentProcess();

   #ifdef _WIN64
   bRunningIn64Bit = TRUE;
   #endif

   #ifdef _AMD64_
   bRunningIn64Bit = TRUE;
   #endif

   #ifdef AMD64
   bRunningIn64Bit = TRUE;
   #endif

   if (bRunningIn64Bit == FALSE)
   {
      moduleHandle = GetModuleHandle(TEXT("kernel32"));

      IsWow64ProcessFunc = (WOW64PROCESS_FUNC)GetProcAddress
                           (
                              moduleHandle,
                              "IsWow64Process"
                           );
      if (IsWow64ProcessFunc == NULL)
      {
         printf("WOW64 info unavailable\n");
         return FALSE;
      }

      if (IsWow64ProcessFunc(myProc, &bRunningIn64Bit) == FALSE)
      {
         printf("Failed to know if running under WOW64\n");
         return FALSE;
      }
   }

   return bRunningIn64Bit;
 
}  // RunningWOW64

// ================ Removal of DevNode ================
VOID ScanAndRemoveDevice(LPCTSTR HwId)
{
   HDEVINFO        devInfoHandle = INVALID_HANDLE_VALUE;
   SP_DEVINFO_DATA devInfoData;
   DWORD           memberIdx = 0;
   CHAR            hardwareIds[REG_HW_ID_SIZE];
   CHAR            compatibleIds[REG_HW_ID_SIZE];
   CHAR            friendlyName[REG_HW_ID_SIZE];
   DWORD           requiredSize;
   BOOL            bResult;
   BOOL            bMatch, bExclude;
   DWORD           errorCode;

   devInfoHandle = SetupDiGetClassDevsEx
                   (
                      NULL,
                      NULL,  // TEXT("USB")
                      NULL,
                      DIGCF_ALLCLASSES,
                      NULL,
                      NULL,  // Machine,
                      NULL
                   );
    if (devInfoHandle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    while (SetupDiEnumDeviceInfo(devInfoHandle, memberIdx, &devInfoData) == TRUE)
    {
       bMatch = bExclude = FALSE;
       ZeroMemory(hardwareIds, REG_HW_ID_SIZE);
       ZeroMemory(compatibleIds, REG_HW_ID_SIZE);
       ZeroMemory(friendlyName, REG_HW_ID_SIZE);

       bResult = SetupDiGetDeviceRegistryProperty
                 (
                    devInfoHandle,
                    &devInfoData,
                    SPDRP_FRIENDLYNAME,
                    NULL,
                    (LPBYTE)friendlyName,
                    REG_HW_ID_SIZE,
                    &requiredSize
                 );
       if (bResult == FALSE)
       {
          SetupDiGetDeviceRegistryProperty
          (
             devInfoHandle,
             &devInfoData,
             SPDRP_DEVICEDESC,
             NULL,
             (LPBYTE)friendlyName,
             REG_HW_ID_SIZE,
             &requiredSize
          );
       }

       bResult = SetupDiGetDeviceRegistryProperty
                 (
                    devInfoHandle,
                    &devInfoData,
                    SPDRP_HARDWAREID,
                    NULL,
                    (LPBYTE)hardwareIds,
                    REG_HW_ID_SIZE,
                    &requiredSize
                 );
       if (bResult == FALSE)
       {
          errorCode = GetLastError();
          if (errorCode != ERROR_INVALID_DATA)
          {
             printf("Error: SetupDiGetDeviceRegistryProperty: hwid (0x%X) reqSZ %d\n",
                     GetLastError(), requiredSize);
          }
       }
       else
       {
          CharUpper((PTSTR)hardwareIds);

          // matching
          if (StrStrW((PTSTR)hardwareIds, HwId) != NULL)
          {
             printf("HWID <%ws>\n", hardwareIds);
             bMatch = TRUE;
          }

          // exclusion
          if (StrStrW((PTSTR)hardwareIds, TEXT(EXCLUDE_PID)) != NULL)
          {
             bExclude = TRUE;
          }
          else if (StrStrW((PTSTR)hardwareIds, TEXT(EXCLUDE_PID2)) != NULL)
          {
             bExclude = TRUE;
          }
       }

       if (bMatch == FALSE)
       {
          bResult = SetupDiGetDeviceRegistryProperty
                    (
                       devInfoHandle,
                       &devInfoData,
                       SPDRP_COMPATIBLEIDS,
                       NULL,
                       (LPBYTE)compatibleIds,
                       REG_HW_ID_SIZE,
                       &requiredSize
                    );
          if (bResult == FALSE)
          {
             errorCode = GetLastError();
             if (errorCode != ERROR_INVALID_DATA)
             {
                printf("Error: SetupDiGetDeviceRegistryProperty: cpid (0x%X) reqSZ %d\n",
                        GetLastError(), requiredSize);
             }
          }
          else
          {
             CharUpper((PTSTR)compatibleIds);

             // matching
             if (StrStrW((PTSTR)compatibleIds, HwId) != NULL)
             {
                printf("COMPAT ID <%ws>\n", hardwareIds);
                bMatch = TRUE;
             }
          }
       }

       if (bMatch == TRUE)
       {
           printf("     <%ws>\n", friendlyName);
           if (gExecutionMode == EXEC_MODE_PREVIEW)
           {
              if (bExclude == FALSE)
              {
                 printf("     ^ to be removed ^\n");
              }
              else
              {
                 printf("     ^ candidate but not to be removed ^\n");
              }
           }
           else
           {
              if (bExclude == FALSE)
              {
                 RemoveDevice(devInfoHandle, &devInfoData);
              }
              else
              {
                 printf("     ^ candidate but not to be removed ^\n");
              }
           }
       }
       memberIdx++;
    }  // while

    if (devInfoHandle != INVALID_HANDLE_VALUE)
    {
        SetupDiDestroyDeviceInfoList(devInfoHandle);
    }

    return;

}  // ScanAndRemoveDevice

BOOL RemoveDevice(HDEVINFO DevInfoHandle, PSP_DEVINFO_DATA DevInfoData)
{
   SP_REMOVEDEVICE_PARAMS removeDevParams;
   SP_DEVINSTALL_PARAMS devInstallParams;
   BOOL bResult = FALSE, bReboot = FALSE;

   removeDevParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
   removeDevParams.Scope = DI_REMOVEDEVICE_GLOBAL;
   removeDevParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
   removeDevParams.HwProfile = 0;

   bResult = SetupDiSetClassInstallParams
             (
                DevInfoHandle,
                DevInfoData,
                &removeDevParams.ClassInstallHeader,
                sizeof(SP_REMOVEDEVICE_PARAMS)
             );
   if (bResult == TRUE)
   {
      bResult = SetupDiCallClassInstaller(DIF_REMOVE, DevInfoHandle, DevInfoData);
      if (bResult == FALSE)
      {
         DWORD err = GetLastError();
         if (err == 0xE0000235)
         {
            printf("Error: executable might not work on 64-bit OS, please run 64-bit executable\n");
         }
         else
         {
            printf("Error: SetupDiCallClassInstaller failure 0x%X\n", err);
         }
      }
   }
   else
   {
      printf("Error: SetupDiSetClassInstallParams failure\n");
   }

   if (bResult == FALSE)
   {
       printf("Error: failed to remove device\n");
   }
   else
   {
       // reboot?
       devInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
       bResult = SetupDiGetDeviceInstallParams
                 (
                    DevInfoHandle,
                    DevInfoData,
                    &devInstallParams
                 );
       if (bResult == TRUE)
       {
          if (devInstallParams.Flags & (DI_NEEDRESTART|DI_NEEDREBOOT))
          {
             bResult = TRUE;
             bReboot = TRUE;
             printf("     Device instance removed successfully, please reboot system\n");
          }
          else
          {
             bResult = TRUE;
             printf("     Device instance removed successfully, no reboot needed\n");
          }
       }
   }
   return bResult;
}  // RemoveDevice

// =================== Reset Service ==================
VOID ResetDriverConfig(VOID)
{
   ResetServiceSettings
   (
      TEXT(SERVICE_KET_SER),
      TEXT("QCDriverSelectiveSuspendIdleTime"),
      0, TRUE
   );
   ResetServiceSettings
   (
      TEXT(SERVICE_KET_SER),
      TEXT("QCDriverWaitWakeEnabled"),
      0, TRUE
   );

   ResetServiceSettings
   (
      TEXT(SERVICE_KET_NET),
      TEXT("QCDriverSelectiveSuspendIdleTime"),
      0, TRUE
   );
   ResetServiceSettings
   (
      TEXT(SERVICE_KET_NET),
      TEXT("QCDriverWaitWakeEnabled"),
      0, TRUE
   );

   ResetServiceSettings
   (
      TEXT(SERVICE_KET_WAN),
      TEXT("QCDriverSelectiveSuspendIdleTime"),
      0, TRUE
   );
   ResetServiceSettings
   (
      TEXT(SERVICE_KET_WAN),
      TEXT("QCDriverWaitWakeEnabled"),
      0, TRUE
   );
}  // ResetDriverConfig

// Either reset an entry or delee it
BOOL ResetServiceSettings
(
   PTSTR ServiceKey,
   PTSTR EntryName,
   DWORD EntryValue,  // ignored if ToRemove is TRUE
   BOOL  ToRemove
)
{
   HKEY hSvcKey;
   BOOL bResult = FALSE;
   DWORD retCode;

   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, ServiceKey, 0, KEY_WRITE, &hSvcKey) == ERROR_SUCCESS)
   {
      if (ToRemove == FALSE)
      {
         retCode = RegSetValueEx
                   (
                      hSvcKey,
                      EntryName,
                      0,            // reserved
                      REG_DWORD,    // type
                      (LPBYTE)&EntryValue,
                      sizeof(DWORD)
                   );
         if (retCode != ERROR_SUCCESS)
         {
            printf("   Error: Cannot reset service settings\n");
         }
         else
         {
            bResult = TRUE;
         }
      }
      else
      {
         retCode = RegDeleteValue(hSvcKey, EntryName);
      }

      RegCloseKey(hSvcKey);
   }
   else
   {
      printf("   Warning: service key not found\n");
   }

   return bResult;
}  // ResetServiceSettings

// ================ Package Removal ===================
VOID PackageRemoval(LPCTSTR PackageName)
{
   WCHAR cmdLineW[MAX_PATH];
   PROCESS_INFORMATION processInfo;
   STARTUPINFO startupInfo;
   BOOL rtnVal;
   DWORD exitCode;

   StringCchCopy(cmdLineW, MAX_PATH, TEXT("wmic product where name=\""));
   StringCchCat(cmdLineW, MAX_PATH, PackageName);
   StringCchCat(cmdLineW, MAX_PATH, TEXT("\" call uninstall"));

   if (gExecutionMode == EXEC_MODE_PREVIEW)
   {
      printf("\n   To remove driver package <%ws>\n", PackageName);
      // printf("   TRACE: <%ws>\n", cmdLineW);
   }
   else
   {
      printf("\nDeleting package <%ws> ...\n", PackageName);

      memset(&processInfo, 0, sizeof(processInfo));
      memset(&startupInfo, 0, sizeof(startupInfo));
      startupInfo.cb = sizeof(startupInfo);

      rtnVal = CreateProcess
               (
                  NULL,
                  cmdLineW,
                  NULL,
                  NULL,
                  FALSE,
                  NORMAL_PRIORITY_CLASS,
                  NULL,
                  NULL,
                  &startupInfo,
                  &processInfo
               );

      if (rtnVal == FALSE)
      {
         printf("Error: CreateProcess failure\n");
      }

      WaitForSingleObject(processInfo.hProcess, INFINITE);
      GetExitCodeProcess(processInfo.hProcess, &exitCode);
      CloseHandle(processInfo.hProcess);
      CloseHandle(processInfo.hThread);
   }

   return;
}  // PackageRemoval

