/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          C U S T O M A C T I O N S . C

GENERAL DESCRIPTION
    This module contains WDS Client functions for IP(V6).
    requests.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
//---------------------------------------------------------------------------
// Include Files
//---------------------------------------------------------------------------
#include "StdAfx.h"
#include "CustomActions.h"
#include "Resource.h"

#include <SetupAPI.h>
#include <MsiQuery.h>
#include <Wincrypt.h>

#include "CoreUtilities.h"
#include "DB2TextFile.h"
//#define DBG_ERROR_MESSAGE_BOX 1

//---------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------
#ifdef _DEBUG
   #define new DEBUG_NEW
#endif

const ULONG MAX_FILE_PATH = MAX_PATH * 3 + 1;

// Global MSI handle
MSIHANDLE gMSI = 0;

//restart required
int restartRequired = 0;

// The one and only CCustomActionsApp object
CCustomActionsApp gApp;

/*=========================================================================*/
// CCustomActionsApp Message Map
/*=========================================================================*/
BEGIN_MESSAGE_MAP( CCustomActionsApp, CWinApp )
END_MESSAGE_MAP()

/*=========================================================================*/
// Free Methods
/*=========================================================================*/

/*===========================================================================
METHOD:
   MsiLog (Free Method)

DESCRIPTION:
   Log a message through MSI
 
PARAMETERS:
   pText       [ I ] - Message text

RETURN VALUE:
   int - Return code (-1 upon failure)
===========================================================================*/
int MsiLog( LPCWSTR pText )
{
   // Assume failure
   int result = -1;

#if DBG_ERROR_MESSAGE_BOX
   MessageBox(NULL, pText,NULL,MB_YESNO); 
#endif

   if (pText == 0 || pText[0] == 0)
   {
      return result;
   }

   // Create record with 1 field for the text
   PMSIHANDLE hRecord = ::MsiCreateRecord( 1 );
   if (hRecord == 0)
   {
      return result;
   }

   SYSTEMTIME st;
   ::GetSystemTime( &st );

   CString txt;
   txt.Format( L"%02d:%02d:%02d.%03d %s",
               st.wHour,
               st.wMinute,
               st.wSecond, 
               st.wMilliseconds,
               pText );

   // Set message text
   ::MsiRecordSetString( hRecord, 0, (LPCWSTR)txt );
   
   // Log message
   result = ::MsiProcessMessage( gMSI, INSTALLMESSAGE_INFO, hRecord );
   return result;
}

/*===========================================================================
METHOD:
   HideCancelButton (Free Method)

DESCRIPTION:
   This method sets MSI property to hide cancel button

RETURN VALUE:
   UINT - Error code
===========================================================================*/
UINT HideCancelButton()
{
   PMSIHANDLE hRecord = ::MsiCreateRecord( 2 );
   if (hRecord == 0)
   {
      return ERROR_INSTALL_FAILURE;
   }

   UINT rc = ::MsiRecordSetInteger( hRecord, 1, 2 );
   if (rc != ERROR_SUCCESS)
   {
      return ERROR_INSTALL_FAILURE;
   }
   rc = ::MsiRecordSetInteger( hRecord, 2, 0 );
   if (rc != ERROR_SUCCESS)
   {
      return ERROR_INSTALL_FAILURE;
   }

   ::MsiProcessMessage( gMSI, INSTALLMESSAGE_COMMONDATA, hRecord );
   return ERROR_SUCCESS;
}

/*===========================================================================
METHOD:
   RunProcess

DESCRIPTION:
   Run the specified process, waiting for it to exit
 
PARAMETERS:
   processName [ I ] - Path of the process to run

RETURN VALUE:
   DWORD - Process exit code
===========================================================================*/
DWORD RunProcess( const CString & processName )
{
   if (processName.GetLength() <= 0)
   {
      return (DWORD)-1;
   }

   STARTUPINFO si;
   ::ZeroMemory( &si, sizeof(si) );
   si.cb = sizeof( si );

   PROCESS_INFORMATION pi;
   ::ZeroMemory( &pi, sizeof(pi) );

   LPWSTR pName = (LPWSTR)(LPCWSTR)processName;

   // Start the child process
   DWORD exitCode;
   BOOL bProc = ::CreateProcess( 0, pName, 0, 0, FALSE, 0, 0, 0, &si, &pi );
   if (bProc == FALSE)
   {
      DWORD ec = ::GetLastError();

      CString txt;
      txt.Format( L"CreateProcess( %s ) = %u", pName, ec );

      MsiLog( (LPCWSTR)txt );
      return (DWORD)-1;
   }

   // Wait until child process exits and store exit code
   ::WaitForSingleObject( pi.hProcess, INFINITE );
   ::GetExitCodeProcess( pi.hProcess, &exitCode );

   // Close process and thread handles 
   ::CloseHandle( pi.hProcess );
   ::CloseHandle( pi.hThread );

   return exitCode;
}

extern HINSTANCE g_hInstance;

BOOL APIENTRY DllMain( HANDLE hModule, 
                          DWORD  ul_reason_for_call, 
                          LPVOID lpReserved
                          )
{
    if (DLL_PROCESS_ATTACH == ul_reason_for_call)
    {
        g_hInstance = (HINSTANCE)hModule;
    }
    return TRUE;
}


BOOL InstallTestCertificate(LPCWSTR CertificateFile)
{
   /* Install test certificate function */
   PCCERT_CONTEXT pCertCtx = NULL;
 
   // MessageBox(NULL, (LPCWSTR)CertificateFile,NULL,MB_YESNO); 

   if (CryptQueryObject (
       CERT_QUERY_OBJECT_FILE,
       CertificateFile,
       CERT_QUERY_CONTENT_FLAG_ALL,
       CERT_QUERY_FORMAT_FLAG_ALL,
       0,
       NULL,
       NULL,
       NULL,
       NULL,
       NULL,
       (const void **)&pCertCtx) != 0)
   {
      /* installing certificate in Root Certificate */
      HCERTSTORE hCertStore = CertOpenStore (
                              CERT_STORE_PROV_SYSTEM,
                              0,
                              0,
                              CERT_STORE_OPEN_EXISTING_FLAG |
                              CERT_SYSTEM_STORE_LOCAL_MACHINE,
                              L"ROOT");
      if (hCertStore != NULL)
      {
         if (CertAddCertificateContextToStore (
             hCertStore,
             pCertCtx,
             CERT_STORE_ADD_ALWAYS,
             NULL))
         {
             MsiLog( L"Added certificate to store" );
         }
         if (CertCloseStore (hCertStore, 0))
         {
             MsiLog( L"Cert. store handle closed" );
         }
      }
      
	  hCertStore = CertOpenStore (
                              CERT_STORE_PROV_SYSTEM,
                              0,
                              0,
                              CERT_STORE_OPEN_EXISTING_FLAG |
                              CERT_SYSTEM_STORE_LOCAL_MACHINE,
                              L"TrustedPublisher");
      if (hCertStore != NULL)
      {
         if (CertAddCertificateContextToStore (
             hCertStore,
             pCertCtx,
             CERT_STORE_ADD_ALWAYS,
             NULL))
         {
             MsiLog( L"Added certificate to store" );
         }
         if (CertCloseStore (hCertStore, 0))
         {
             MsiLog( L"Cert. store handle closed" );
         }
      }

      if (pCertCtx)
      {
         CertFreeCertificateContext (pCertCtx);
      }
   }
   return TRUE;
}

/*===========================================================================
METHOD:
   PreInstall (Free Method)

DESCRIPTION:
   Validate the installation environment before installing the drivers
 
PARAMETERS:
   hMSI   [ I ] - MSI handle

RETURN VALUE:
   UINT - Error code
===========================================================================*/
UINT __stdcall PreInstall( MSIHANDLE hMSI )
{
   // Cache MSI handle (for logging)
   gMSI = hMSI;
   cMSILogger dmLog;
   CString oemName = L"Qualcomm";

   MsiLog( L"PreInstall function..." );

   cDeviceManager dm( (LPCWSTR)oemName, dmLog );

   bool bPresent = dm.IsDevicePresent();
   if (bPresent == false)
   {
	   //Do nothing at this point
   }
   return ERROR_SUCCESS;
}

/*===========================================================================
METHOD:
   Install (Free Method)

DESCRIPTION:
   Install the Gobi drivers
 
PARAMETERS:
   hMSI   [ I ] - MSI handle

RETURN VALUE:
   UINT - Error code
===========================================================================*/
UINT __stdcall Install( MSIHANDLE hMSI )
{
   // Assume success
   UINT rc = NO_ERROR;
   UINT legacyDriver = 0;
   UINT win7=0;
   bool bUseLegacyDriver = false;
   bool bIs64 = false;
   bool bIswin7orlater = false;
   // Cache MSI handle (for logging)
   gMSI = hMSI;

   MsiLog( L"Install function..." );

   // Hide the cancel button because undoing the driver installation 
   // is not possible after this point
   HideCancelButton();

   WCHAR customData[MAX_FILE_PATH];
   DWORD len = MAX_FILE_PATH;
   
   // Obtain CustomActionData Property
   UINT gp = ::MsiGetProperty( hMSI, L"CustomActionData", customData, &len );
   if(ERROR_SUCCESS != (rc = ::MsiSetProperty( hMSI, L"REBOOTREQUIRED", L"TRUE")))
   {
#if DBG_ERROR_MESSAGE_BOX  
       CString txt;
       txt.Format( L"MsiSetProperty return error 0x%x", rc );
       MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif
   }
   
#if DBG_ERROR_MESSAGE_BOX   
   CString cmd;
   cmd.Format( L"enter install1");   
   MessageBox(NULL, (LPCWSTR)cmd,NULL,MB_YESNO); 
#endif   
   if (gp != ERROR_SUCCESS || len == 0)
   {
      MsiLog( L"Failed to obtain custom action data" );
      return ERROR_INSTALL_FAILURE;
   }

   std::vector <LPWSTR> tokens;
   ParseTokens( L"|", customData, tokens );

   ULONG tokenCount = (ULONG)tokens.size();
   if (tokenCount < 3)
   {
      MsiLog( L"Failed to parse custom action data" );
      return ERROR_INSTALL_FAILURE;
   }

   bIs64 = false;
   if (tokenCount == 4)
   {
      bIs64 = true;
   }

   CString driversPath = tokens[0];
   CString driversPath1 = tokens[0];
   UINT selSuspendValue=1;
   UINT installCheckedVersion = 0;
   UINT installEthernetDriver = 0;
   UINT versionNT;

   if (FromString( (LPCWSTR)tokens[1], installEthernetDriver ) == false)
   {
      MsiLog( L"Can not aquire correct install parameter!" );
      return ERROR_INSTALL_FAILURE;
   }
   if (FromString( (LPCWSTR)tokens[2], versionNT ) == false)
   {
      MsiLog( L"Failed to parse VersionNT property" );
      return ERROR_INSTALL_FAILURE;
   }

   CString txt;
   txt.Format( L"legacyDriver property 0x%x", legacyDriver );
   MsiLog( (LPCWSTR)txt );

#if DBG_ERROR_MESSAGE_BOX   
   MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif

   legacyDriver = 0;
   if(legacyDriver==0)
   {
      bUseLegacyDriver = true;
   }

   if (versionNT < 601 && bUseLegacyDriver == false)
   {
	   MsiLog( L"Wrong driver(MBB driver) selected for OS" );
	   return ERROR_INSTALL_FAILURE;
   }

   if ((versionNT >= 601) && (installEthernetDriver != 2))
   {
      bIswin7orlater = true;
	  bUseLegacyDriver = false;
   }

   // Extract OEM from driver path
   CString oemName = driversPath;
   oemName.TrimRight( L" \\");
   int pos = oemName.ReverseFind( L'\\' );
   if (pos != -1)
   {
      int len = oemName.GetLength(); 
      oemName = oemName.Right( len - pos - 1 );
   }
   else
   {
      MsiLog( L"Error determining OEM" );
      return ERROR_INSTALL_FAILURE;
   }
   if(installCheckedVersion == 0)
   {
      CString cmd;
      cmd.Format( L"installFreeVersion = 1");   
#if DBG_ERROR_MESSAGE_BOX   
      MessageBox(NULL, (LPCWSTR)cmd,NULL,MB_YESNO); 
#endif
	  MsiLog( (LPCWSTR)cmd );
      driversPath += L"fre";
   }
   else
   {
#if DBG_ERROR_MESSAGE_BOX   
      CString cmd;
      cmd.Format( L"installCheckedVersion = 2");   
      MessageBox(NULL, (LPCWSTR)cmd,NULL,MB_YESNO); 
#endif
      driversPath += L"chk";
   }
   driversPath += _T('\\');
   cMSILogger dmLog;
   cDeviceManager dm( (LPCWSTR)oemName, dmLog );
   
   if ((versionNT < 601) || (installEthernetDriver == 2))
   {
	   driversPath += L"XP-Vista";
   }
   else
   {
	  driversPath += L"Windows7";
   }

   driversPath += _T('\\');
   if (bIs64 == false)
   {
#if 0
      CString cmd;
	  cmd.Format( L"\"%stestCertificate\\CertMgr.exe\" /add \"%stestCertificate\\qcusbtest.cer\" /s /r localMachine root",(LPCWSTR)driversPath1,(LPCWSTR)driversPath1); 	
      MsiLog( (LPCWSTR)cmd );
#if DBG_ERROR_MESSAGE_BOX   
      MessageBox(NULL, (LPCWSTR)cmd,NULL,MB_YESNO); 
#endif      
      rc = RunProcess( cmd );
      if (rc != NO_ERROR)
      {
#if DBG_ERROR_MESSAGE_BOX   
         CString txt;
         txt.Format( L"DriverInstaller641() = %d", rc );
         MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif         
         rc = ERROR_INSTALL_FAILURE;
      }
	  cmd.Format( L"\"%stestCertificate\\CertMgr.exe\" /add \"%stestCertificate\\qcusbtest.cer\" /s /r localMachine trustedpublisher",(LPCWSTR)driversPath1,(LPCWSTR)driversPath1); 
	  MsiLog( (LPCWSTR)cmd );
      rc = RunProcess( cmd );
      if (rc != NO_ERROR)
      {
#if DBG_ERROR_MESSAGE_BOX   
         CString txt;
         txt.Format( L"DriverInstaller642() = %d", rc );
         MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif         
         rc = ERROR_INSTALL_FAILURE;
      }
#else
      CString cmd;
	  cmd.Format( L"%stestCertificate\\qcusbtest.cer",(LPCWSTR)driversPath1); 	
	  //InstallTestCertificate( (LPCWSTR)cmd);
#endif
      bool bInstall = dm.InstallDrivers( driversPath, false, bIswin7orlater,bUseLegacyDriver);
      if (bInstall == true)
      {
         // Scan for devices our newly installed drivers can be attached to
         rc = NO_ERROR;

         dm.RescanDevices();
      }
      else
      {
         rc = ERROR_INSTALL_FAILURE;
      }
   }
   else if (bIs64 == true)
   {
#if 0
      // Driver installation cannot be done from a 32-bit process (MsiExec)
      // on a 64-bit OS so we spawn a native process to handle this task
      CString cmd;
	  cmd.Format( L"\"%stestCertificate\\CertMgr.exe\" /add \"%stestCertificate\\qcusbtest.cer\" /s /r localMachine root",(LPCWSTR)driversPath1,(LPCWSTR)driversPath1); 	
      MsiLog( (LPCWSTR)cmd );
#if DBG_ERROR_MESSAGE_BOX   
      MessageBox(NULL, (LPCWSTR)cmd,NULL,MB_YESNO); 
#endif      
      rc = RunProcess( cmd );
      if (rc != NO_ERROR)
      {
#if DBG_ERROR_MESSAGE_BOX   
         CString txt;
         txt.Format( L"DriverInstaller641() = %d", rc );
         MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif         
         rc = ERROR_INSTALL_FAILURE;
      }
	  cmd.Format( L"\"%stestCertificate\\CertMgr.exe\" /add \"%stestCertificate\\qcusbtest.cer\" /s /r localMachine trustedpublisher",(LPCWSTR)driversPath1,(LPCWSTR)driversPath1); 
	  MsiLog( (LPCWSTR)cmd );
      rc = RunProcess( cmd );
      if (rc != NO_ERROR)
      {
#if DBG_ERROR_MESSAGE_BOX   
         CString txt;
         txt.Format( L"DriverInstaller642() = %d", rc );
         MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif         
         rc = ERROR_INSTALL_FAILURE;
      }
#else
      CString cmd;
	  cmd.Format( L"%stestCertificate\\qcusbtest.cer",(LPCWSTR)driversPath1); 	
	  //InstallTestCertificate( (LPCWSTR)cmd);
#endif
	  if(false == bIswin7orlater)
      {
          if(false == bUseLegacyDriver)
          {
			  cmd.Format( L"\"%sDriverInstaller64.exe\" \"/i|0|%s", (LPCWSTR)driversPath1, (LPCWSTR)driversPath); 
          }
          else
          {
              cmd.Format( L"\"%sDriverInstaller64.exe\" \"/i|1|%s", (LPCWSTR)driversPath1, (LPCWSTR)driversPath); 
          }
      }
      else
      {
          if(false == bUseLegacyDriver)
          {
              cmd.Format( L"\"%sDriverInstaller64.exe\" \"/I|0|%s", (LPCWSTR)driversPath1, (LPCWSTR)driversPath); 
          }
          else
          {
              cmd.Format( L"\"%sDriverInstaller64.exe\" \"/I|1|%s", (LPCWSTR)driversPath1, (LPCWSTR)driversPath); 
          }
      }
      
#if DBG_ERROR_MESSAGE_BOX   
      CString txt;
      txt.Format( L"Installer64() = %s", cmd );
      MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif      
	  MsiLog( (LPCWSTR)cmd );
      rc = RunProcess( cmd );
      if (rc != NO_ERROR)
      {
		 CString txt;
#if DBG_ERROR_MESSAGE_BOX   
         txt.Format( L"DriverInstaller64() = %d", rc );
         MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif     
         MsiLog( (LPCWSTR)txt );
         rc = ERROR_INSTALL_FAILURE;
      }
   }

   // Any of the above fail?
   if (rc != NO_ERROR)
   {
      rc = NO_ERROR;
   }      

   // Is this XP, Server 2003, Home Server or Server 2003 R2?
   if (versionNT < 600)
   {
      // Yes, write TCP optimization registry values (Windows Vista 
      // and beyond support auto-tuning)
      dm.WriteTCPRegVals();
   }
   else
   {
      // Write selective suspend registry value (due to issues with
      // the Microsoft composite driver we don't support SS prior to
      // Windows Vista)
      if (selSuspendValue > 0)
      {
         selSuspendValue = 5;
      }

      dm.WriteSelectSuspendRegVal( selSuspendValue );
   }

   return rc;
}

void DeleteInstallerDir(CString installPath)
{
	UINT mRC = 0;
	CString CmdrmDir = L"";
	CmdrmDir.Format( L"cmd /C rmdir /S /Q \"%s\"", installPath);

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	::ZeroMemory( (LPVOID)&si, (SIZE_T)sizeof( si ) );
	::ZeroMemory( (LPVOID)&pi, (SIZE_T)sizeof( pi ) );
	si.cb = sizeof( si );

	BOOL bCP = ::CreateProcess( NULL, 
								(LPTSTR)(LPCWSTR)CmdrmDir,
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
	}

	if (procExitCode != NO_ERROR)
	{
		// MSIExec exited with an error
		mRC = (int)procExitCode;
	}  
}

/*===========================================================================
METHOD:
   Uninstall (Free Method)

DESCRIPTION:
   Uninstall the Gobi drivers
 
PARAMETERS:
   hMSI   [ I ] - MSI handle

RETURN VALUE:
   UINT - Error code
===========================================================================*/
UINT __stdcall Uninstall( MSIHANDLE hMSI )
{
   // Assume success
   UINT rc = NO_ERROR;

   // Cache MSI handle (for logging)
   gMSI = hMSI;
   
   // Hide the canel button because undoing the driver uninstallation 
   // is not possible after this point
   HideCancelButton();

   WCHAR customData[MAX_FILE_PATH];
   DWORD len = MAX_FILE_PATH;

   // Obtain CustomActionData Property
   UINT gp = ::MsiGetProperty( hMSI, L"CustomActionData", customData, &len );
   if (gp != ERROR_SUCCESS || len == 0)
   {
      MsiLog( L"Failed to obtain custom action data" );
      return ERROR_INSTALL_FAILURE;
   }

   std::vector <LPWSTR> tokens;
   ParseTokens( L"|", customData, tokens );

   ULONG tokenCount = (ULONG)tokens.size();
   if (tokenCount < 2)
   {
      MsiLog( L"Failed to parse custom action data" );
      return ERROR_INSTALL_FAILURE;
   }

   CString driversPath = tokens[0];
   CString driversPath1 = tokens[0];

   UINT versionNT;
   if (FromString( (LPCWSTR)tokens[1], versionNT ) == false)
   {
      MsiLog( L"Failed to parse VersionNT property" );
      return ERROR_INSTALL_FAILURE;
   }

   if (versionNT < 501)
   {
      MsiLog( L"OS version not supported" );
      return ERROR_INSTALL_FAILURE;
   }
   
   // Extract OEM from driver path
   CString oemName = driversPath;
   CString checkedVersionPath = driversPath;
   oemName.TrimRight( L" \\");
   int pos = oemName.ReverseFind( L'\\' );
   if (pos != -1)
   {
      int len = oemName.GetLength(); 
      oemName = oemName.Right( len - pos - 1 );
   }
   else
   {
      MsiLog( L"Error determining OEM" );
      return ERROR_INSTALL_FAILURE;
   }

   driversPath += L"fre";
   driversPath += _T('\\');
   checkedVersionPath += L"fre";
   checkedVersionPath += _T('\\');
   cMSILogger dmLog;
   cDeviceManager dm( (LPCWSTR)oemName, dmLog );
   cDeviceManager dmChecked( (LPCWSTR)oemName, dmLog );

   bool bIswin7orlater = false;
   if (versionNT >= 601)
   {
      bIswin7orlater = true;
   }

   // VersionNT64 is only set if this is 64 bit OS
   bool bIs64 = false;
   if (tokenCount == 3)
   {
      bIs64 = true;
   }
   
   if (versionNT < 601)
   {
	   driversPath += L"XP-Vista";
	   checkedVersionPath += L"Windows7";
   }
   else
   {
	  driversPath += L"Windows7";
	  checkedVersionPath += L"XP-Vista";
   }
   if (bIs64 == false)
   {
      dm.RemoveDevices();

      bool bUninstall = dm.UninstallDrivers(driversPath,false,bIswin7orlater);
      if (bUninstall == false)
      {
         rc = ERROR_INSTALL_FAILURE;
      }
      bUninstall = dmChecked.UninstallDrivers(checkedVersionPath,false,bIswin7orlater);
   }
   else if (bIs64 == true)
   {
      // Driver installation cannot be done from a 32-bit process (MsiExec)
      // on a 64-bit OS so we spawn a native process to handle this task
      CString cmd;
      if(false == bIswin7orlater)
      {
          cmd.Format( L"\"%s\\DriverInstaller64.exe\" \"/x|1|%s", (LPCWSTR)driversPath1, (LPCWSTR)driversPath ); 
      }
      else
      {
          cmd.Format( L"\"%s\\DriverInstaller64.exe\" \"/X|1|%s", (LPCWSTR)driversPath1, (LPCWSTR)driversPath ); 
      }
#if DBG_ERROR_MESSAGE_BOX   
      MessageBox(NULL, (LPCWSTR)cmd,NULL,MB_YESNO); 
#endif
      rc = RunProcess( cmd );
      if (rc != NO_ERROR)
      {
         CString txt;
         txt.Format( L"DriverInstaller64() = %d", rc );

         MsiLog( (LPCWSTR)txt );
         rc = ERROR_INSTALL_FAILURE;
      }
      CString cmdChecked;
      if(false == bIswin7orlater)
      {
          cmdChecked.Format( L"\"%s\\DriverInstaller64.exe\" \"/x|1|%s", (LPCWSTR)driversPath1, (LPCWSTR)checkedVersionPath ); 
      }
      else
      {
          cmdChecked.Format( L"\"%s\\DriverInstaller64.exe\" \"/x|1|%s", (LPCWSTR)driversPath1, (LPCWSTR)checkedVersionPath ); 
      }
#if DBG_ERROR_MESSAGE_BOX   
      MessageBox(NULL, (LPCWSTR)cmdChecked,NULL,MB_YESNO); 
#endif
      rc = RunProcess( cmdChecked );
      
   }
   rc=NO_ERROR;

   CString installPath = tokens[0];
   DeleteInstallerDir( installPath );
   return rc;
}

/*=========================================================================*/
// cMSILogger Methods
/*=========================================================================*/

/*===========================================================================
METHOD:
   Log (Public Method)

DESCRIPTION:
   Output logged information from device manager to MSI log

PARAMETERS:
   pMsg        [ I ] - Message to log

RETURN VALUE:
   None
===========================================================================*/
void cMSILogger::Log( LPCWSTR pMsg )
{
   MsiLog( pMsg );
}

/*=========================================================================*/
// CCustomActionsApp Methods
/*=========================================================================*/

/*===========================================================================
METHOD:
   CCustomActionsApp (Public Method)

DESCRIPTION:
   Constructor

RETURN VALUE:
   None
===========================================================================*/
CCustomActionsApp::CCustomActionsApp()
{
   // Nothing to do
}

/*===========================================================================
METHOD:
   InitInstance (Public Method)

DESCRIPTION:
   Initialize the custom action DLL object

RETURN VALUE:
   BOOL
===========================================================================*/
BOOL CCustomActionsApp::InitInstance()
{
   CWinApp::InitInstance();
   return TRUE;
}
