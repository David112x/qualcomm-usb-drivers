/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          STDAFX. H

GENERAL DESCRIPTION

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================
FILE: 
   StdAfx.h

DESCRIPTION:
   Precompiled headers include file

PUBLIC CLASSES AND FUNCTIONS:
   None

Copyright (C) 2010 Qualcomm Technologies Incorporated. All rights reserved.
                   QUALCOMM Proprietary/GTDR

All data and information contained in or disclosed by this document is
confidential and proprietary information of Qualcomm Technologies Incorporated and all
rights therein are expressly reserved.  By accepting this material the
recipient agrees that this material and the information contained therein
is held in confidence and in trust and will not be used, copied, reproduced
in whole or in part, nor its contents revealed in any manner to others
without the express written permission of Qualcomm Technologies Incorporated.
===========================================================================*/

//---------------------------------------------------------------------------
// Pragmas
//---------------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------
#ifndef VC_EXTRALEAN
   // Exclude rarely-used stuff from Windows headers
   #define VC_EXTRALEAN
#endif

// Some CString constructors will be explicit
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

//---------------------------------------------------------------------------
// Include Files
//---------------------------------------------------------------------------
#include "TargetVer.h"

// MFC core and standard components
#include <AfxWin.h>
#include <AfxExt.h>

#ifdef _UNICODE
   #if defined _M_IX86
      #pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
   #elif defined _M_IA64
      #pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
   #elif defined _M_X64
      #pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
   #else
      #pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
   #endif
#endif




