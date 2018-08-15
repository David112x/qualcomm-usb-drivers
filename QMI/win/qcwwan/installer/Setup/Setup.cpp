/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          S E T U P . C P P 

GENERAL DESCRIPTION

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================
FILE: 
   Setup.cpp

DESCRIPTION:
   CSetupApp's InitInstance checks the command line options:
      /i or /x                   - Install(default) or uninstall
      /l                         - Turn on logging
      /q                         - Quiet (no UI) install/uninstall
      /?                         - Displays which options are available
      [PROPERTY=PropertyValue]   - Sets a public property

PUBLIC CLASSES AND FUNCTIONS:
   CSetupApp

Copyright (C) 2010 QUALCOMM Incorporated. All rights reserved.
                   QUALCOMM Proprietary/GTDR

All data and information contained in or disclosed by this document is
confidential and proprietary information of QUALCOMM Incorporated and all
rights therein are expressly reserved.  By accepting this material the
recipient agrees that this material and the information contained therein
is held in confidence and in trust and will not be used, copied, reproduced
in whole or in part, nor its contents revealed in any manner to others
without the express written permission of QUALCOMM Incorporated.
==========================================================================*/

//---------------------------------------------------------------------------
// Include Files
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include <Strsafe.h>
#include "Setup.h"
#include "SetupCommandLineInfo.h"
#include "CoreUtilities.h"
#include "DB2TextFile.h"

#include <Shellapi.h>
//---------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------
#ifdef _DEBUG
   #define new DEBUG_NEW
#endif

typedef BOOL (WINAPI *PFUNCPTR)(PVOID *);
typedef BOOL (WINAPI *PFUNCPTR1)(PVOID);

// Exit codes
const int SETUP_RETURN_SUCCESS         = 0;
const int SETUP_RETURN_INTERNAL        = -1;
const int SETUP_RETURN_BAD_COMMAND     = -2;
const int SETUP_RETURN_MISSING_FILES   = -3;

CSetupApp theApp;

/*=========================================================================*/
// CSetupApp Message Map
/*=========================================================================*/
BEGIN_MESSAGE_MAP( CSetupApp, CWinApp )
END_MESSAGE_MAP()

/*===========================================================================
METHOD:
   CSetupApp

DESCRIPTION:
   Constructor for CSetupApp class

RETURN VALUE:
   None
===========================================================================*/
CSetupApp::CSetupApp()
   :  mRC ( SETUP_RETURN_INTERNAL )
{
   mUsageMessage = L"Setup.exe [Optional Parameters]\n\n"
                   L"Optional Parameters:\n"
                   L"   /i          Install\n"
                   L"   /x          Uninstall\n"
                   L"   /l          Turn on logging\n"
                   L"   /q          Quiet (No UI)\n"
                   L"   /?          Help\n\n"
                   L"Setting Public Properties:\n"
                   L"   [PROPERTY=PropertyValue]";

}

/*===========================================================================
METHOD:
   ExtractResource

DESCRIPTION:
   Extract the specified resource to the given location

PARAMETERS
   pRscTable   [ I ] - Name of resource table to load from
   rscID       [ I ] - Resource ID
   pFilePath   [ I ] - Fully qualified path of file to extract resource to

RETURN VALUE:
   None
===========================================================================*/
DWORD ExtractResource(
   LPCWSTR                    pRscTable,
   DWORD                      rscID,
   LPCWSTR                    pFilePath )
{
   HINSTANCE hApp = ::AfxGetResourceHandle();
   HRSRC hRes = ::FindResource( (HMODULE)hApp, 
                                MAKEINTRESOURCE( rscID ), 
                                pRscTable );

   if (hRes == 0)
   {
      return ::GetLastError();
   }

   DWORD resSz = ::SizeofResource( (HMODULE)hApp, hRes );
   if (resSz == 0)
   {
      return ::GetLastError();
   }

   HGLOBAL hGlobal = ::LoadResource( (HMODULE)hApp, hRes );
   if (hGlobal == 0)
   {
      return ::GetLastError();
   }

   BYTE * pData = (BYTE *)::LockResource( hGlobal );
   if (pData == 0)
   {
      return ::GetLastError();
   }

   // Attempt to open the file
   HANDLE hFile = ::CreateFile( pFilePath, 
                                GENERIC_WRITE, 
                                FILE_SHARE_READ,
                                0, 
                                CREATE_ALWAYS, 
                                FILE_ATTRIBUTE_NORMAL, 
                                0 );

   if (hFile == INVALID_HANDLE_VALUE)
   {
      return ::GetLastError();
   }

   // Assume the write succeeds
   DWORD rc = NO_ERROR;

   DWORD bytesWritten = 0;
   BOOL bWrite = ::WriteFile( hFile, (LPCVOID)pData, resSz, &bytesWritten, 0 );
   if (bWrite == FALSE)
   {
      rc = ::GetLastError();
   }

   // Close the file
   ::CloseHandle( hFile );
   hFile = INVALID_HANDLE_VALUE;

   return rc;
}
/*===========================================================================
METHOD:
   DisplayError (Internal Method)
   
DESCRIPTION:
   Display an error to the user (accounting for silent install)

PARAMETERS
   pError      [ I ] - Error to display

RETURN VALUE:
   None
===========================================================================*/
void CSetupApp::DisplayError( LPCWSTR pError )
{
   if (pError == 0 || pError[0] == 0)
   {
      return;
   }

   AfxMessageBox( pError );
}

// ================ Package Removal ===================
VOID PackageRemoval(LPCTSTR PackageName)
{
#if 0
   WCHAR cmdLineW[MAX_PATH];
   PROCESS_INFORMATION processInfo;
   STARTUPINFO startupInfo;
   BOOL rtnVal;
   DWORD exitCode;

   StringCchCopy(cmdLineW, MAX_PATH, TEXT("wmic product where name=\""));
   StringCchCat(cmdLineW, MAX_PATH, PackageName);
   StringCchCat(cmdLineW, MAX_PATH, TEXT("\" call uninstall"));

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
   return;
#endif
   CString msiCmd = L"msiexec.exe /x {D9FB7F91-9687-4B09-894D-072903CADEA4} /passive";

   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   ::ZeroMemory( (LPVOID)&si, (SIZE_T)sizeof( si ) );
   ::ZeroMemory( (LPVOID)&pi, (SIZE_T)sizeof( pi ) );
   si.cb = sizeof( si );

   BOOL bCP = ::CreateProcess( NULL, 
                               (LPTSTR)(LPCWSTR)msiCmd,
                               0,
                               0,
                               FALSE,
                               0, 
                               0,
                               0,
                               &si,
                               &pi );

   if (bCP == FALSE)
   {
      // Unable to create MSIExec process
      return;
   }

   // Wait until MSIExec process exits
   ::WaitForSingleObject( pi.hProcess, INFINITE );

   // Get process exit code
   DWORD procExitCode = NO_ERROR;
   BOOL bEC = ::GetExitCodeProcess( pi.hProcess, &procExitCode );

   // Close process and thread handles
   ::CloseHandle( pi.hProcess );
   ::CloseHandle( pi.hThread );

   if (bEC == FALSE)
   {
      // Unable to obtain MSIExec process exit code
      return;
   }

   if (procExitCode != NO_ERROR)
   {
      // MSIExec exited with an error
      return;
   }
   return;
}  // PackageRemoval

/*===========================================================================
METHOD:
   InitInstance
   
DESCRIPTION:
   Reads command line parameters to determine if options /i, /x, /q, /l and /? 
   were given.  User can also specify properties in the format 
   [PROPERTY=PropertyValue] to be passed along to the msiexec call.

RETURN VALUE:
   BOOL
===========================================================================*/
BOOL CSetupApp::InitInstance()
{
   // Set the destination for the embedded MSI
   CString msiPath;
   CString msg = L"";

   // Parse command line for command line arguments
   CSetupCommandLineInfo cmdInfo;
   ParseCommandLine( cmdInfo );

   bool bIsValid = cmdInfo.IsValidCommandLine();
   if (bIsValid != true)
   {
      mRC = SETUP_RETURN_BAD_COMMAND;
      AfxMessageBox( mUsageMessage );
      return FALSE;
   }

   // Do they want to know which commands are available?
   bool bHelp = cmdInfo.IsOptionSet( SETUP_OPT_HELP );
   if (bHelp == true)
   {
      mRC = SETUP_RETURN_SUCCESS;
      AfxMessageBox( mUsageMessage );
      return FALSE;
   }

   // Do they want to uninstall?
   CString cmd = SETUP_OPT_I;
   bool bUninstall = cmdInfo.IsOptionSet( SETUP_OPT_X );
   if (bUninstall == true)
   {
      cmd = SETUP_OPT_X;
   }
   
   //extract the MSI file
   {
      // Grab the temporary folder path
      WCHAR tempPath[MAX_PATH * 2 + 1];
      tempPath[0] = 0;

      DWORD err = ::GetTempPath( MAX_PATH * 2, tempPath );
      if (err == 0)
      {
         DWORD ec = (int)::GetLastError();
         msg.Format( L"Unable to obtain temporary path, %u", ec );
         DisplayError( (LPCWSTR)msg );

         mRC = SETUP_RETURN_INTERNAL;
         return FALSE;
      }

      // Does it actually exist?
      err = ::GetFileAttributes( tempPath );
      if (err == INVALID_FILE_ATTRIBUTES)
      {
         mRC = SETUP_RETURN_INTERNAL;
         return FALSE;
      }

      // Set the destination for the embedded MSI
      msiPath = tempPath;
      msiPath += L"QualcommWindowsDriverInstaller.msi";

      // Extract the MSI to the temporary folder
      DWORD ec = ExtractResource( L"MSI", IDR_MSI, (LPCWSTR)msiPath );
      if (ec != NO_ERROR)
      {
         msg.Format( L"Error extracting MSI, %u", ec );
         DisplayError( (LPCWSTR)msg );

         mRC = SETUP_RETURN_INTERNAL;
         return FALSE;
      }
   }

   CString opt = L"";

   // Do they want to turn on logging?
   bool bLogging = cmdInfo.IsOptionSet( SETUP_OPT_L );
   if (bLogging == true)
   {
      WCHAR tempPath[MAX_PATH * 2 + 1];
      tempPath[0] = 0;

      DWORD err = ::GetTempPath( MAX_PATH * 2, tempPath );
      if (err != 0)
      {
         err = ::GetFileAttributes( tempPath );
         if (err != INVALID_FILE_ATTRIBUTES)
         {
            // Create unique filename
            WCHAR uniqueFile[MAX_PATH * 2 + 1];
            UINT rc = ::GetTempFileName( (LPCWSTR)&tempPath[0],
                                         L"usbdrivers",
                                         0,
                                         uniqueFile );

            if (rc != 0)
            {
               opt = SETUP_OPT_L;
               opt += L" ";
               opt += (LPCWSTR)&uniqueFile[0]; 
               opt += L" ";
            }
         }
      }
   }

   // Do they want a silent install?
   bool bSilent = cmdInfo.IsOptionSet( SETUP_OPT_Q );
   if (bSilent == true)
   {
      opt += SETUP_OPT_Q;
   }

   // Add REBOOTNEEDED flag based on testsigning flag.
   {
      WCHAR uniqueFile[MAX_PATH * 2 + 1];
      WCHAR tempPath[MAX_PATH * 2 + 1];
      tempPath[0] = 0;

      DWORD err = ::GetTempPath( MAX_PATH * 2, tempPath );
      if (err != 0)
      {
         err = ::GetFileAttributes( tempPath );
         if (err != INVALID_FILE_ATTRIBUTES)
         {
            // Create unique filename
            ::GetTempFileName( (LPCWSTR)&tempPath[0],
                               L"usbdrivers",
                               0,
                               uniqueFile );

         }
      }

      CString cmd;
	  PVOID tempVal;
	  PFUNCPTR pFuncPtr;
	  pFuncPtr = (PFUNCPTR) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "Wow64DisableWow64FsRedirection");

#if 0
	  if ( (pFuncPtr != NULL) && (pFuncPtr(&tempVal)) )
	  {
         //Check whether test signing is on or not 
         cmd.Format(L"cmd.exe /c bcdedit.exe > \"%s\"", (LPCWSTR)uniqueFile); 

	     STARTUPINFO si;
         PROCESS_INFORMATION pi;

         ::ZeroMemory( (LPVOID)&si, (SIZE_T)sizeof( si ) );
         ::ZeroMemory( (LPVOID)&pi, (SIZE_T)sizeof( pi ) );
         si.cb = sizeof( si );
      
         BOOL bCP = ::CreateProcess( NULL, 
                                     (LPWSTR)(LPCWSTR)cmd,
								     0,
								     0,
								     FALSE,
								     0, 
								     0,
								     0,
								     &si,
								     &pi );

         if (bCP == FALSE)
         {
            // Unable to create MSIExec process
         }
	     else
	     {
            // Wait until MSIExec process exits
            ::WaitForSingleObject( pi.hProcess, INFINITE );

            // Get process exit code
            DWORD procExitCode = NO_ERROR;
            BOOL bEC = ::GetExitCodeProcess( pi.hProcess, &procExitCode );

            // Close process and thread handles
            ::CloseHandle( pi.hProcess );
            ::CloseHandle( pi.hThread );

            if (bEC == FALSE)
            {
               // Unable to obtain MSIExec process exit code
               bCP = FALSE;
            }

            if (procExitCode != NO_ERROR)
            {
               // MSIExec exited with an error
               bCP = FALSE;
            }
         
            UINT restartRequired = 1;
            if (bCP == TRUE)
            {
               CString cmd;
               cmd.Format(uniqueFile); 
               cDB2TextFile inFile( cmd );
               if (inFile.IsValid() == true)
               {
                  CString line;
                  while (inFile.ReadLine( line ) == true)
                  {
                     CString lineCopy = line;
                     LPTSTR pLine = lineCopy.GetBuffer( lineCopy.GetLength() );
			         if (wcsstr((LPCWSTR)pLine, L"testsigning") && wcsstr((LPCWSTR)pLine, L"Yes"))
			         {
			            restartRequired = 0;
					    break;
			         }
		          }
		          DeleteFile(uniqueFile);
	           }
	           else
	           {
				  restartRequired = 0;
			   } 
            }
		    else
		    {
			   restartRequired = 0;
		    }
	        if (restartRequired == 1)
	        {

               //Check whether test signing is on or not 
               cmd.Format(L"bcdedit.exe /set testsigning on"); 

	           STARTUPINFO si;
	           PROCESS_INFORMATION pi;

	           ::ZeroMemory( (LPVOID)&si, (SIZE_T)sizeof( si ) );
	           ::ZeroMemory( (LPVOID)&pi, (SIZE_T)sizeof( pi ) );
	           si.cb = sizeof( si );
      
	           BOOL bCP = ::CreateProcess( NULL, 
                                           (LPWSTR)(LPCWSTR)cmd,
								           0,
								           0,
								           FALSE,
								           0, 
								           0,
								           0,
								           &si,
								           &pi );

               if (bCP == FALSE)
               {
                  // Unable to create MSIExec process
               }
	           else
	           {
                  // Wait until MSIExec process exits
                  ::WaitForSingleObject( pi.hProcess, INFINITE );

                  // Get process exit code
                  DWORD procExitCode = NO_ERROR;
                  BOOL bEC = ::GetExitCodeProcess( pi.hProcess, &procExitCode );

                  // Close process and thread handles
                  ::CloseHandle( pi.hProcess );
                  ::CloseHandle( pi.hThread );

                  if (bEC == FALSE)
                  {
                     // Unable to obtain MSIExec process exit code
                     bCP = FALSE;
                  }

                  if (procExitCode != NO_ERROR)
                  {
                     // MSIExec exited with an error
                     bCP = FALSE;
                  }
                  
				  CString rebootreqd = L"";
                  rebootreqd.Format( L" REBOOTNEEDED=%d", restartRequired);

                  opt += rebootreqd;
	           }
		    }
         }
	  }
#endif
	  PFUNCPTR1 pFuncPtr1;
  	  pFuncPtr1 = (PFUNCPTR1) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "Wow64RevertWow64FsRedirection");
	  if (pFuncPtr1 != NULL)
	  {
         // pFuncPtr1(tempVal);
	  }
   }


   // Remove Old Package
   PackageRemoval(TEXT("Qualcomm USB Drivers For Windows"));

   // Add optional parameters
   opt += cmdInfo.FormatProperties();

   CString msiCmd = L"";
   msiCmd.Format( L"msiexec.exe %s \"%s\" %s",
                  cmd,
                  (LPCWSTR)msiPath,
                  opt );

   STARTUPINFO si;
   PROCESS_INFORMATION pi;

   ::ZeroMemory( (LPVOID)&si, (SIZE_T)sizeof( si ) );
   ::ZeroMemory( (LPVOID)&pi, (SIZE_T)sizeof( pi ) );
   si.cb = sizeof( si );

   BOOL bCP = ::CreateProcess( NULL, 
                               (LPTSTR)(LPCWSTR)msiCmd,
                               0,
                               0,
                               FALSE,
                               0, 
                               0,
                               0,
                               &si,
                               &pi );

   if (bCP == FALSE)
   {
      // Unable to create MSIExec process
      mRC = (int)::GetLastError();
      return FALSE;
   }

   // Wait until MSIExec process exits
   ::WaitForSingleObject( pi.hProcess, INFINITE );

   // Get process exit code
   DWORD procExitCode = NO_ERROR;
   BOOL bEC = ::GetExitCodeProcess( pi.hProcess, &procExitCode );

   // Close process and thread handles
   ::CloseHandle( pi.hProcess );
   ::CloseHandle( pi.hThread );

   if (bEC == FALSE)
   {
      // Unable to obtain MSIExec process exit code
      mRC = (int)::GetLastError();
      return FALSE;
   }

   if (procExitCode != NO_ERROR)
   {
      // MSIExec exited with an error
      mRC = (int)procExitCode;
      return FALSE;
   }

   DeleteFile((LPCWSTR)msiPath);

   mRC = SETUP_RETURN_SUCCESS;
   return FALSE;
}

/*===========================================================================
METHOD:
   ExitInstance
   
DESCRIPTION:
   Returns appropriate return code

RETURN VALUE:
   int - Process return code
===========================================================================*/
int CSetupApp::ExitInstance() 
{
   CWinApp::ExitInstance();
   return mRC;
}


