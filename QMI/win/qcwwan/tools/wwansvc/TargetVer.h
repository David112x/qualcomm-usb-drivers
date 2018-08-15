/*===========================================================================
FILE: 
   TargetVer.h

DESCRIPTION:
   Precompiled headers include file

PUBLIC CLASSES AND FUNCTIONS:
   None

Copyright (C) 2011 QUALCOMM Incorporated. All rights reserved.
                   QUALCOMM Proprietary/GTDR

All data and information contained in or disclosed by this document is
confidential and proprietary information of QUALCOMM Incorporated and all
rights therein are expressly reserved.  By accepting this material the
recipient agrees that this material and the information contained therein
is held in confidence and in trust and will not be used, copied, reproduced
in whole or in part, nor its contents revealed in any manner to others
without the express written permission of QUALCOMM Incorporated.
===========================================================================*/

//---------------------------------------------------------------------------
// Pragmas
//---------------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------
#ifndef WINVER 
   #define WINVER 0x0501
#endif

#ifndef _WIN32_WINNT
   #define _WIN32_WINNT 0x0501
#endif

#ifndef _WIN32_WINDOWS
   #define _WIN32_WINDOWS 0x0410
#endif

#ifndef _WIN32_IE
   #define _WIN32_IE 0x0700
#endif
