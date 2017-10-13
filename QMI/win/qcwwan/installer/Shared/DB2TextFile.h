/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          DB2TEXTFILE . C

GENERAL DESCRIPTION

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

//---------------------------------------------------------------------------
// Pragmas
//---------------------------------------------------------------------------
#pragma once

/*=========================================================================*/
// Class cDB2TextFile
/*=========================================================================*/
class cDB2TextFile
{
   public:
      // Constructor (loads file)
      cDB2TextFile( LPCTSTR pMemFile );

      // Constructor (loads file from resource table)
      cDB2TextFile( 
         HMODULE                    hModule, 
         LPCTSTR                    pRscTable,
         DWORD                      rscID );

      // Constructor (loads file from buffer)
      cDB2TextFile( 
         BYTE *                     pBuffer,
         ULONG                      bufferLen );

      // Destructor
      ~cDB2TextFile();

      // (Inline) Get the file name
      LPCTSTR GetFileName()
      {
         return mFileName;
      };

      // (Inline) Get error status
      DWORD GetStatus()
      {
         return mStatus;
      };

      // (Inline) Get the file contents
      bool GetText( CString & copy )
      {
         bool bOK = IsValid();
         if (bOK == true)
         {
            copy = mText;
         }

         return bOK;
      };

      // (Inline) Get file validity
      virtual bool IsValid()
      {
         return (mStatus == NO_ERROR);
      };

      // Read the next available line
      bool ReadLine( CString & line );

   protected:
      /* File contents (converted to ANSI/UNICODE) */
      CString mText;

      /* Current position (in above contents) */
      int mCurrentPos;

      /* Error status */
      DWORD mStatus;

      /* The name of the file */
      TCHAR mFileName[MAX_PATH * 2 + 1];
};
