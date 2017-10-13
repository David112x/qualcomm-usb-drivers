/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          C U S T O M A C T I O N S . H

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

//---------------------------------------------------------------------------
// Include Files
//---------------------------------------------------------------------------
#include "Resource.h"
#include "DeviceManagement.h"

//#define DBG_ERROR_MESSAGE_BOX 1  /*Should be set to 0 in formal release*/

//---------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------

/*=========================================================================*/
// Class cMSILogger
//    Class to output logged information from device manager to MSI log
/*=========================================================================*/
class cMSILogger : public cDeviceManagerLogger
{
   public:
      // (Inline) Constructor
      cMSILogger()
      { };

      // (Inline) Destructor
      virtual ~cMSILogger()
      { };

      // Log a message
      virtual void Log( LPCWSTR pMsg );
};

/*=========================================================================*/
// Class CCustomActionsApp
/*=========================================================================*/
class CCustomActionsApp : public CWinApp
{
   public:
      // Constructor
      CCustomActionsApp();

      // Initialize the custom action DLL object
      virtual BOOL InitInstance();

   DECLARE_MESSAGE_MAP()
};
