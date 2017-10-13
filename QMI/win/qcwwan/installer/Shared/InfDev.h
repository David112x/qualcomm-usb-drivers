/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                           I N F D E V. H

GENERAL DESCRIPTION
  Scan and remove USB VID_06C5 related INF files from system

  Copyright (c) 2015 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#ifndef INFDEV_H
#define INFDEV_H

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <string.h>
#include <winioctl.h>
#include <setupapi.h>
#include <Strsafe.h>
#include <Shlwapi.h>

extern INT gExecutionMode;

#define QC_INF_PATH L"C:\\Windows\\Inf"
// #define QC_INF_PATH L"C:\\Work\\Tools\\QDCLR\\v4"

#define EXEC_MODE_PREVIEW    1
#define EXEC_MODE_REMOVE_OEM 2
#define EXEC_MODE_REMOVE_ALL 3

#define INF_SIZE_MAX (1024*1024)
#define SECTION_MAX 10000
#define SECTION_TABLE_MAX 1000
#define LINE_LEN_MAX 2048

#define MATCH_VID    "VID_05C6"
#define MATCH_PID    "PID_9001"
#define MATCH_PID2   "PID_9049"
#define MATCH_PID3   "PID_9025"
#define EXCLUDE_PID  "PID_9204"
#define EXCLUDE_PID2 "PID_9205"

#define REG_HW_ID_SIZE 2048
#define ALL_INF "\\*.inf"
#define OEM_INF "\\oem*.inf"

#define SERVICE_KET_SER "SYSTEM\\CurrentControlSet\\Services\\qcusbser"
#define SERVICE_KET_NET "SYSTEM\\CurrentControlSet\\Services\\qcusbnet"
#define SERVICE_KET_WAN "SYSTEM\\CurrentControlSet\\Services\\qcusbwwan"

typedef BOOL (WINAPI *WOW64PROCESS_FUNC)(HANDLE, PBOOL);

// Function Prototypes
VOID DisplayTime(PCHAR Title, BOOL NewLine);
INT  MainRemovalTask(VOID);
BOOL CheckOSCompatibility(VOID);

// INF/driver removal
VOID ScanInfFiles(LPCTSTR InfPath);
BOOL MatchingInf(PCTSTR InfFullPath, PCTSTR InfFileName, ULONG DataSize);
BOOL ConfirmInfFile(PCTSTR InfFullPath);
BOOL DeviceMatch(PTSTR InfText, DWORD TextSize);
VOID InfRemoval(LPCTSTR FilePath, PCTSTR InfFileName, PCHAR Type);
BOOL IsOSVersionVistaOrHigher(VOID);
BOOL RunningWOW64(VOID);

// Dev node removal
VOID ScanAndRemoveDevice(LPCTSTR HwId);
RemoveDevice(HDEVINFO DevInfoHandle, PSP_DEVINFO_DATA DevInfoData);

// Driver config reset
VOID ResetDriverConfig(VOID);
BOOL ResetServiceSettings
(
   PTSTR ServiceKey,
   PTSTR EntryName,
   DWORD EntryValue,  // ignored if ToRemove is TRUE
   BOOL  ToRemove
);

// QC driver package removal
VOID PackageRemoval(LPCTSTR PackageName);

#endif // INFDEV_H
