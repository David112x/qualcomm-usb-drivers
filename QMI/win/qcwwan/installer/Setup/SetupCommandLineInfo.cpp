/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          SETUPCOMMANDLINE . CPP

GENERAL DESCRIPTION

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================
FILE: 
   SetupCommandLineInfo.cpp

DESCRIPTION:
   Implemetation of CSetupCommandLineInfo class
   
   This class represents a command line parser where the arguments can
   consist of (/i or /x), /l and /q.  It is also permissible to not include
   any arguments.  User can also specify properties in the format 
   [PROPERTY=PropertyValue] to be passed along to the msiexec call.

PUBLIC CLASSES AND METHODS:
   CSetupCommandLineInfo

Copyright (C) 2008 QUALCOMM Incorporated. All rights reserved.
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
// Include Files
//---------------------------------------------------------------------------
#include "Stdafx.h"
#include "SetupCommandLineInfo.h"

//---------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------
#ifdef _DEBUG
   #define new DEBUG_NEW
#endif

// Option lookup strings
const LPCWSTR SETUP_OPT_I    = L"/i";
const LPCWSTR SETUP_OPT_X    = L"/x";
const LPCWSTR SETUP_OPT_Q    = L"/qn";
const LPCWSTR SETUP_OPT_L    = L"/l*";
const LPCWSTR SETUP_OPT_HELP = L"/?";


/*===========================================================================
METHOD:
   CSetupCommandLineInfo

DESCRIPTION:
   Constructor for CSetupCommandLineInfo class

PARAMETERS:
   None
  
RETURN VALUE:
   None
===========================================================================*/
CSetupCommandLineInfo::CSetupCommandLineInfo() 
   :  CCommandLineInfo(),
      mbValidCommandLine( true ),
      mState( eSTATE_BEGIN )
{
   // Nothing to do
}

/*===========================================================================
METHOD:
   CSetupCommandLineInfo

DESCRIPTION:
   Destructor for CSetupCommandLineInfo class
  
RETURN VALUE:
   None
===========================================================================*/
CSetupCommandLineInfo::~CSetupCommandLineInfo()
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
void CSetupCommandLineInfo::ParseParam( 
   LPCWSTR                 pParam, 
   BOOL                 /* bFlag */, 
   BOOL                    bLast )
{
   // Don't bother parsing if MFC has gone crazy
   if (pParam == 0)
   {
      mState = eSTATE_ERROR;
      mCommands.clear();

      return;
   }

   // Don't bother parsing if an error has been detected
   if (mState == eSTATE_ERROR)
   {
      return;
   }

   CString OPT_TRUE = L"true";
   if (_wcsicmp( pParam, L"i" ) == 0)
   {          
     mState = eSTATE_VALID;
     mCommands[SETUP_OPT_I] = OPT_TRUE;
   }
   else if (_wcsicmp( pParam, L"x" ) == 0)
   {          
      mState = eSTATE_VALID;
      mCommands[SETUP_OPT_X] = OPT_TRUE;
   }
   else if (_wcsicmp( pParam, L"l" ) == 0)
   { 
      mState = eSTATE_VALID;
      mCommands[SETUP_OPT_L] = OPT_TRUE;
   }
   else if (_wcsicmp( pParam, L"q" ) == 0)
   { 
      mState = eSTATE_VALID;
      mCommands[SETUP_OPT_Q] = OPT_TRUE;
   }
   else if (_wcsicmp( pParam, L"?" ) == 0)
   { 
      mState = eSTATE_VALID;
      mCommands[SETUP_OPT_HELP] = OPT_TRUE;
   }
   else
   {
      // Valid Parameters must be in the format [PROPERTY=PropertyValue]
      CString temp(pParam);
      int loc = temp.Find( L"=" );
      if (loc != -1 && loc != 0 && loc != temp.GetLength() - 1)
      {
         mProperties.push_back( pParam );
      }
      // Just ignore the rest
   }

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
         mState = eSTATE_ERROR;
         mCommands.clear();
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
bool CSetupCommandLineInfo::CheckParameters()
{
   // Assume success
   bool bOK = true;

   // Check that parsing suceeded
   if (mState == eSTATE_ERROR)
   {
      bOK = false;
      return bOK;
   }

   // These options are mutually exclusive
   if ( (IsOptionSet( SETUP_OPT_I ) == true)
   &&   (IsOptionSet( SETUP_OPT_X ) == true) )
   {
      bOK = false;
   }

   return bOK;
}

