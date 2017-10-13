/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          SETUPCOMMANDLINEINFO. H

GENERAL DESCRIPTION

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*===========================================================================
FILE: 
   SetupCommandLineInfo.h

DESCRIPTION:
   Declaration of CSetupCommandLineInfo class

PUBLIC CLASSES AND METHODS:
   CSetupCommandLineInfo

   This class represents a command line parser where the arguments can
   consist of (/i or /x), /l and /q.  It is also permissible to not include
   any arguments.  User can also specify properties in the format 
   [PROPERTY=PropertyValue] to be passed along to the msiexec call.

Copyright (C) 2008 Qualcomm Technologies Incorporated. All rights reserved.
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
// Include Files
//---------------------------------------------------------------------------
#include <map>
#include <vector>

// Option lookup strings
extern const LPCWSTR SETUP_OPT_I;
extern const LPCWSTR SETUP_OPT_X;
extern const LPCWSTR SETUP_OPT_Q;
extern const LPCWSTR SETUP_OPT_L;
extern const LPCWSTR SETUP_OPT_HELP;

/*=========================================================================*/
// Class CSetupCommandLineInfo
/*=========================================================================*/
class CSetupCommandLineInfo : public CCommandLineInfo
{
   public:
      // Constructor
      CSetupCommandLineInfo();

      // Destructor
      ~CSetupCommandLineInfo();

      // (Inline) Is the given option set?
      bool IsOptionSet( LPCWSTR pOpt ) const
      {
         bool bOpt = (mCommands.find( pOpt ) != mCommands.end());
         return bOpt;
      };

      // (Inline) Is this a valid command line?
      bool IsValidCommandLine()
      {
         return mbValidCommandLine;
      };

      // (Inline) Format the properties into a string
      CString FormatProperties()
      {
         CString propString = L"";

         ULONG sz = (ULONG)mProperties.size();
         for (ULONG i = 0; i < sz; i++)
         {
            propString += L" ";
            propString += mProperties[i];
         }

         return propString;
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

      /* Command line parsing states */
      enum eState
      {
         eSTATE_BEGIN,

         eSTATE_VALID,   // Valid parameter was last parsed
         eSTATE_ERROR,   // Error parsing parameters

         eSTATE_END
      };

      /* The current state */
      eState mState;

      /* Parsed commands */
      std::map <LPCWSTR, CString> mCommands;

      /* Optional properties */
      std::vector <CString> mProperties;
};
