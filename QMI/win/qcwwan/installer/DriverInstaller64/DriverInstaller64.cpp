/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          M P I P . C

GENERAL DESCRIPTION

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
//---------------------------------------------------------------------------
// Include Files
//---------------------------------------------------------------------------
#include "StdAfx.h"

#include "DriverInstaller64.h"
#include "CoreUtilities.h"

//#define DBG_ERROR_MESSAGE_BOX 1

//---------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------
#ifdef _DEBUG
   #define new DEBUG_NEW
#endif
#define AUTO_CLOSE_UNSIGNED_WARN 0
int restartRequired=0;
// Exit codes
const int RETURN_SUCCESS = 0;
const int RETURN_FAILURE = 1;
const int RETURN_BAD_COMMAND = 2;

CDriverInstaller64App gApp;

/*=========================================================================*/
// CDriverInstaller64App Message Map
/*=========================================================================*/
BEGIN_MESSAGE_MAP( CDriverInstaller64App, CWinApp )
END_MESSAGE_MAP()

/*=========================================================================*/
// CDriverCommandLineInfo Methods
/*=========================================================================*/

/*===========================================================================
METHOD:
   CDriverCommandLineInfo

DESCRIPTION:
   Constructor for CDriverCommandLineInfo class

PARAMETERS:
   None
  
RETURN VALUE:
   None
===========================================================================*/
CDriverCommandLineInfo::CDriverCommandLineInfo() 
   :  CCommandLineInfo(),
      mbValidCommandLine( false ),
      mbInstall( false ),
      mbUninstall( false ),
      mbiswin7orlater(false),
      mbInvalid( false )
{
   // Nothing to do
}

/*===========================================================================
METHOD:
   CDriverCommandLineInfo

DESCRIPTION:
   Destructor for CDriverCommandLineInfo class
  
RETURN VALUE:
   None
===========================================================================*/
CDriverCommandLineInfo::~CDriverCommandLineInfo()
{
   // Nothing to do
}

/*===========================================================================
METHOD:
   ParseParam

DESCRIPTION:
   Parse an individual parameters

PARAMETERS:
   pParam         [ I ] - What to parse
   bFlag          [ I ] - Is this a flag?
   bLast          [ I ] - Last parameter?

RETURN VALUE:
   None
===========================================================================*/
void CDriverCommandLineInfo::ParseParam( 
   LPCWSTR                 pParam, 
   BOOL                 /* bFlag */, 
   BOOL                    bLast )
{
    std::vector <LPWSTR> tokens;
   // Don't bother parsing if MFC has gone crazy
   if (pParam == 0)
   {
      return;
   }
#if DBG_ERROR_MESSAGE_BOX   
   MessageBox(NULL, pParam,NULL,MB_YESNO);   
#endif
   ParseTokens( L"|", (LPTSTR)pParam, tokens );
   ULONG tokenCount = (ULONG)tokens.size();
   if (tokenCount < 3)
   {
#if DBG_ERROR_MESSAGE_BOX   
      MessageBox(NULL, L"Failed to parse commandLineInfo",NULL,MB_YESNO); 
#endif      
   }

   CString installFlag = tokens[0];
   UINT legacyUsbEthernetDriver=1;
#if DBG_ERROR_MESSAGE_BOX   
         MessageBox(NULL, installFlag,NULL,MB_YESNO); 
#endif      
   if (wcscmp( installFlag, L"i" ) == 0)
   {          
      mbInstall = true;
   }
   else if (wcscmp( installFlag, L"x" ) == 0)
   {          
      mbUninstall = true;
   }
   else if (_wcsicmp( installFlag, L"I" ) == 0)
   {          
      mbInstall = true;
      mbiswin7orlater=true;
   }
   else if (_wcsicmp( installFlag, L"X" ) == 0)
   {          
      mbUninstall = true;
      mbiswin7orlater=true;
   }
   else
   {
      mbInvalid = true;
   }
   mbisLegacyEthernetDriver = true;
   if(true == mbiswin7orlater) 
   {
      if (FromString( (LPCWSTR)tokens[1], legacyUsbEthernetDriver ) == false)
      {
#if DBG_ERROR_MESSAGE_BOX   
         MessageBox(NULL, L"Failed to parse legacyUsbEthernetDriver property",NULL,MB_YESNO);   
#endif
      }
#if DBG_ERROR_MESSAGE_BOX   
               MessageBox(NULL, tokens[1],NULL,MB_YESNO); 
#endif      
      if(0 == legacyUsbEthernetDriver)
      {
#if DBG_ERROR_MESSAGE_BOX   
         MessageBox(NULL, L"use WWAN driver",NULL,MB_YESNO);  
#endif         
          mbisLegacyEthernetDriver = false;
      }
   }

   installPath = tokens[2];
#if DBG_ERROR_MESSAGE_BOX   
   MessageBox(NULL, tokens[2],NULL,MB_YESNO); 
#endif 

   // Set status on last parameter
   if (bLast != FALSE)
   {
      if (CheckParameters() == true)
      {
         mbValidCommandLine = true;
      }
      else
      {
         mbValidCommandLine = false;
      }
   }
}

/*===========================================================================
METHOD:
   CheckParameters

DESCRIPTION:
   Checks that parameters are valid
  
RETURN VALUE:
   bool
===========================================================================*/
bool CDriverCommandLineInfo::CheckParameters()
{
   // Assume success
   bool bOK = true;

   // These options are mutually exclusive
   if (mbInstall == true && mbUninstall == true)
   {
      bOK = false;
   }

   //Invalid Command check
   if (mbInvalid == true)
   {
      bOK = false;
   }

   return bOK;
}

/*=========================================================================*/
// CDriverInstaller64App Methods
/*=========================================================================*/

/*===========================================================================
METHOD:
   CDriverInstaller64App

DESCRIPTION:
   Constructor for CDriverInstaller64App class

RETURN VALUE:
   None
===========================================================================*/
CDriverInstaller64App::CDriverInstaller64App()
   :  mRC ( RETURN_FAILURE )
{
   // Nothing to do
}

/*===========================================================================
METHOD:
   InitInstance
   
DESCRIPTION:
   Reads command line parameters to determine if options /i and /x

RETURN VALUE:
   BOOL
===========================================================================*/
BOOL CDriverInstaller64App::InitInstance()
{
#if DBG_ERROR_MESSAGE_BOX   
    CString txt;
    txt.Format( L"CDriverInstaller64App::InitInstance2");
    MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif    
   if (CWinApp::InitInstance() == FALSE)
   {
      return FALSE;
   }

   // Parse command line for command line arguments
   CDriverCommandLineInfo cmdInfo;
   ParseCommandLine( cmdInfo );

   bool bIsValid = cmdInfo.IsValidCommandLine();
   if (bIsValid != true)
   {
      mRC = RETURN_BAD_COMMAND;
#if DBG_ERROR_MESSAGE_BOX   
          CString txt;
          txt.Format( L"CDriverInstaller64App::InitInstance RETURN_BAD_COMMAND");
          MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif    
      return FALSE;
   }

   // Get the module name
   WCHAR path[MAX_PATH * 2 + 1];
   DWORD rc = ::GetModuleFileName( 0, &path[0], MAX_PATH );
   if (rc <= 0)
   {
#if DBG_ERROR_MESSAGE_BOX   
                 CString txt;
                 txt.Format( L"CDriverInstaller64App::InitInstance GetModuleFileName %s",path);
                 MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif    
      return FALSE;
   }

   // Find last backslash
   CString driversPath = path;
   int pos = driversPath.ReverseFind( L'\\' );
   if (pos == -1)
   {
#if DBG_ERROR_MESSAGE_BOX   
                        CString txt;
                        txt.Format( L"CDriverInstaller64App::InitInstance ReverseFind");
                        MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif    
      return FALSE;
   }

   // Remove module name
   driversPath = driversPath.Left( pos );

   // Find last backslash
   pos = driversPath.ReverseFind( L'\\' );
   if (pos == -1)
   {
#if DBG_ERROR_MESSAGE_BOX   
                               CString txt;
                               txt.Format( L"CDriverInstaller64App::InitInstance ReverseFind2");
                               MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif    
      return FALSE;
   }

   // Remove OS folder
   driversPath = driversPath.Left( pos );

   // Extract OEM from driver path
   CString oemName = driversPath;
   pos = oemName.ReverseFind( L'\\' );
   if (pos == -1)
   {
#if DBG_ERROR_MESSAGE_BOX   
                                      CString txt;
                                      txt.Format( L"CDriverInstaller64App::InitInstance ReverseFind3");
                                      MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif    
      return FALSE;
   }

   int len = oemName.GetLength(); 
   oemName = oemName.Right( len - pos - 1 );

   cDebugLogger dmLog;
   cDeviceManager dm( (LPCWSTR)oemName, dmLog );

   if (cmdInfo.IsInstall() == true)
   {	  
#ifdef _WIN64
	   bool bInstall = dm.InstallDrivers(cmdInfo.CommandInstallPath(), true, cmdInfo.IsWin7(), cmdInfo.IsLegacyEthernetDriver());
#else
	   bool bInstall = dm.InstallDrivers(cmdInfo.CommandInstallPath(), false, cmdInfo.IsWin7(), cmdInfo.IsLegacyEthernetDriver());
#endif
      
      if (bInstall == true)
      {
         // Scan for devices our newly installed drivers can be attached to
         dm.RescanDevices();

         mRC = RETURN_SUCCESS;
      }
   }
   else if (cmdInfo.IsUninstall() == true)
   {
      dm.RemoveDevices();
#ifdef _WIN64
	  bool bUninstall = dm.UninstallDrivers(cmdInfo.CommandInstallPath(), true, cmdInfo.IsWin7());
#else
	  bool bUninstall = dm.UninstallDrivers(cmdInfo.CommandInstallPath(), false, cmdInfo.IsWin7());
#endif
      
      if (bUninstall == true)
      {
         mRC = RETURN_SUCCESS;
      }
   }

   return TRUE;
}

/*===========================================================================
METHOD:
   ExitInstance
   
DESCRIPTION:
   Returns appropriate return code

RETURN VALUE:
   int - Process return code
===========================================================================*/
int CDriverInstaller64App::ExitInstance() 
{
   CWinApp::ExitInstance();
   return mRC;
}


