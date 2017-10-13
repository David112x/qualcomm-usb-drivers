/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          S T D A F X . C

GENERAL DESCRIPTION

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
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

// some CString constructors will be explicit
#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS

//---------------------------------------------------------------------------
// Include Files
//---------------------------------------------------------------------------
#include "targetver.h"
#include <afxwin.h>
#include <afxext.h>

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>
#include <afxodlgs.h>
#include <afxdisp.h>
#endif

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>
#endif

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>
#endif

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>
#endif


