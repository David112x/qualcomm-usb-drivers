/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                         S E T U P . C

GENERAL DESCRIPTION

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================
FILE: 
   Setup.cpp

DESCRIPTION:
   CSetupApp's InitInstance that checks the command line options:
      /i or /x                   - Install(default) or uninstall
      /l                         - Turn on logging
      /q                         - Quiet (no UI) install/uninstall
      /?                         - Displays which options are available
      [PROPERTY=PropertyValue]   - Sets a public property

   User can also specify properties in the format A=B to be passed along to
   the msiexec call.

PUBLIC CLASSES AND FUNCTIONS:
   CSetupApp

Copyright (C) 2008 QUALCOMM Incorporated. All rights reserved.
                   QUALCOMM Proprietary/GTDR

All data and information contained in or disclosed by this document is
confidential and proprietary information of QUALCOMM Incorporated and all
rights therein are expressly reserved.  By accepting this material the
recipient agrees that this material and the information contained therein
is held in confidence and in trust and will not be used, copied, reproduced
in whole or in part, nor its contents revealed in any manner to others
without the express written permission of QUALCOMM Incorporated.
==========================================================================*/

#pragma once

#ifndef __AFXWIN_H__
   #error "include 'Stdafx.h' before including this file for PCH"
#endif

//---------------------------------------------------------------------------
// Include Files
//---------------------------------------------------------------------------
#include "Resource.h"
#include <vector>
#include <map>

/*=========================================================================*/
// Class CSetupApp
/*=========================================================================*/
class CSetupApp : public CWinApp
{
   public:
      // Default constructor
      CSetupApp();

      // Initialize application instance
      virtual BOOL InitInstance();

      // Cleanup application instance
      virtual int ExitInstance();

   protected:
      /* Usage string */
      CString mUsageMessage;

      /* Return code */
      int mRC;

      // Display an error to the user (accounting for silent install)
      void DisplayError( LPCWSTR pError );

   DECLARE_MESSAGE_MAP()
};

extern CSetupApp theApp;
