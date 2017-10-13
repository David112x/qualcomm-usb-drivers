/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          M P I P . C

GENERAL DESCRIPTION

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

//---------------------------------------------------------------------------
// Pragmas
//---------------------------------------------------------------------------
#pragma once

#ifndef __AFXWIN_H__
   #error "include 'stdafx.h' before including this file for PCH"
#endif

#define NOCOINSTALLER 1

//---------------------------------------------------------------------------
// Include Files
//---------------------------------------------------------------------------
#include "Resource.h"
#include "DeviceManagement.h"

//---------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------

/*=========================================================================*/
// Class cDebugLogger
//    Class to output logged information from device manager to debug log
/*=========================================================================*/
class cDebugLogger : public cDeviceManagerLogger
{
   public:
      // (Inline) Constructor
      cDebugLogger()
      { };

      // (Inline) Destructor
      virtual ~cDebugLogger()
      { };

      // Log a message
      virtual void Log( LPCWSTR pMsg )
      {
         if (pMsg != 0 && pMsg[0] != 0)
         {
            ::OutputDebugString( pMsg );
         }
      };
};

/*=========================================================================*/
// Class CDriverCommandLineInfo
/*=========================================================================*/
class CDriverCommandLineInfo : public CCommandLineInfo
{
   public:
      // Constructor
      CDriverCommandLineInfo();

      // Destructor
      ~CDriverCommandLineInfo();

      // (Inline) Is this a valid command line?
      bool IsValidCommandLine()
      {
         return mbValidCommandLine;
      };

      // (Inline) Is the command Install?
      bool IsInstall()
      {
         return mbInstall;
      };

      // (Inline) Is the command Uninstall?
      bool IsUninstall()
      {
         return mbUninstall;
      };

      bool IsWin7()
	  {
		 return mbiswin7orlater;
	  };

      CString CommandInstallPath()
	  {
		 return installPath;
	  };

	  bool IsLegacyEthernetDriver()
	  {
          return mbisLegacyEthernetDriver;
	  };

      // Parse an individual parameter or flag
      virtual void ParseParam( 
         LPCWSTR                 pParam, 
         BOOL                    bFlag, 
         BOOL                    bLast );

   protected:
      // Check validity of parameters
      virtual bool CheckParameters();

      /* Valid command line? */
      bool mbValidCommandLine;

      /* install command */
      bool mbInstall;

      /* Uninstall command */
      bool mbUninstall;

      /* is WIN7 */
      bool mbiswin7orlater;

      /*is legacy NDIS5.1 ethernet driver*/
      bool mbisLegacyEthernetDriver;
      
      /* Invalid command */
      bool mbInvalid;

	  /* install path */
	  CString installPath;

};

/*=========================================================================*/
// Class CDriverInstaller64App
/*=========================================================================*/
class CDriverInstaller64App : public CWinApp
{
   public:
      // Constructor
      CDriverInstaller64App();

      // Initialize the application
      virtual BOOL InitInstance();

      // Cleanup application instance
      virtual int ExitInstance();

   protected:
         /* Return code */
         int mRC;

   DECLARE_MESSAGE_MAP()
};

extern CDriverInstaller64App gApp;
