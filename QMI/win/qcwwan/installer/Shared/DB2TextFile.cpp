/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          DB2TEXTFILE . C

GENERAL DESCRIPTION

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
//-----------------------------------------------------------------------------
// Include Files
//-----------------------------------------------------------------------------
#include "Stdafx.h"
#include "DB2TextFile.h"

//-----------------------------------------------------------------------------
// Definitions
//-----------------------------------------------------------------------------
#ifdef _DEBUG
   #define new DEBUG_NEW
#endif

// Maximum size of a file this interface will try to open (8 MB)
const DWORD MAX_FILE_SZ = 1024 * 1024 * 8;

// Maximum number of characters to run UNICODE check over
const INT MAX_UNICODE_CHECK_LEN = 128;

/*=========================================================================*/
// cDB2TextFile Methods
/*=========================================================================*/

cDB2TextFile::cDB2TextFile( LPCTSTR pFile )
   :  mText( _T("") ),
      mStatus( ERROR_FILE_NOT_FOUND ),
      mCurrentPos( 0 )
{
   if (pFile == 0 || pFile[0] == 0)
   {
      return;
   }

   // Copy filename
   _tcsncpy_s( mFileName, (MAX_PATH * 2), pFile, _TRUNCATE );

   // Open the file
   HANDLE hFile = ::CreateFile( pFile,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                0,
                                OPEN_EXISTING,
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                0 );

   if (hFile == INVALID_HANDLE_VALUE)
   {
      mStatus = ::GetLastError();
      return;
   }

   // Grab the file size
   DWORD fileSize = ::GetFileSize( hFile, 0 );
   if (fileSize == INVALID_FILE_SIZE || fileSize > MAX_FILE_SZ)
   {
      ::CloseHandle( hFile );
      mStatus = ERROR_GEN_FAILURE;
      return;
   }

   // Create mapping object
   HANDLE hMapping = ::CreateFileMapping( hFile, 0, PAGE_READONLY, 0, 0, 0 );
   if (hMapping == 0)
   {
      mStatus = ::GetLastError();
      ::CloseHandle( hFile );  
      return;
   }

   // Map file in
   LPVOID pBuffer = ::MapViewOfFile( hMapping, 
                                     FILE_MAP_READ,
                                     0,
                                     0,
                                     (SIZE_T)fileSize );

   if (pBuffer == 0)
   {
      mStatus = ::GetLastError();

      ::CloseHandle( hMapping );
      ::CloseHandle( hFile );
      return;
   }

   PBYTE pStr = (PBYTE)pBuffer;

   // Is this text UNICODE?
   bool bUNICODE = false;

   // For starters it has to be of even length
   if ((fileSize % 2) == 0)
   {
      // Better let Windows take over here
      INT flags = IS_TEXT_UNICODE_STATISTICS 
                | IS_TEXT_UNICODE_REVERSE_SIGNATURE
                | IS_TEXT_UNICODE_SIGNATURE;

      INT checkLen = (INT)fileSize;
      if (checkLen > MAX_UNICODE_CHECK_LEN)
      {
         checkLen = MAX_UNICODE_CHECK_LEN;
      }

      BOOL bUNI = ::IsTextUnicode( (LPCVOID)pStr, checkLen, &flags );
      if (bUNI != FALSE)
      {
         bUNICODE = true;

         if ( (flags & IS_TEXT_UNICODE_SIGNATURE)
         ||   (flags & IS_TEXT_UNICODE_REVERSE_SIGNATURE) )
         {
            // File starts with a UNICODE BOM, skip it
            ULONG charSz = (ULONG)sizeof(USHORT);
            pStr += charSz;
            fileSize -= charSz;
         }
      }
   }

#ifdef UNICODE
            
   if (bUNICODE == true)
   {
      // UNICODE item string, UNICODE core - no problem
      fileSize /= (DWORD)sizeof(USHORT);
      mText = CString( (LPWSTR)pStr, fileSize );
   }
   else
   {
      // ANSI item string, UNICODE core - conversion needed
      mText = CString( (LPSTR)pStr, fileSize );
   }

#else
            
   if (bUNICODE == true)
   {
      // UNICODE item string, ANSI core - conversion needed
      fileSize /= (DWORD)sizeof(USHORT);
      mText = CString( (LPWSTR)pStr, fileSize );
   }
   else
   {
      // ANSI item string, ANSI core - no problem
      mText = CString( (LPSTR)pStr, fileSize );
   }

#endif

   ::UnmapViewOfFile( pBuffer );
   ::CloseHandle( hMapping );
   ::CloseHandle( hFile );

   // Success!
   mStatus = NO_ERROR;
}

/*===========================================================================
METHOD:
   cDB2TextFile (Public Method)

DESCRIPTION:
   Construct object/load resource into memory

PARAMETERS
   hModule     [ I ] - Module to load resources from
   pRscTable   [ I ] - Name of resource table to load from
   rscID       [ I ] - Resource ID

RETURN VALUE:
   None
===========================================================================*/
cDB2TextFile::cDB2TextFile(
   HMODULE                    hModule, 
   LPCTSTR                    pRscTable,
   DWORD                      rscID )
   :  mText( _T("") ),
      mStatus( ERROR_FILE_NOT_FOUND ),
      mCurrentPos( 0 )
{
   HRSRC hRes = ::FindResource( hModule, 
                                MAKEINTRESOURCE( rscID ), 
                                pRscTable );

   if (hRes == 0)
   {
      mStatus = ::GetLastError();
      return;
   }

   DWORD resSz = ::SizeofResource( hModule, hRes );
   if (resSz == 0)
   {
      mStatus = ::GetLastError();
      return;
   }

   HGLOBAL hGlobal = ::LoadResource( hModule, hRes );
   if (hGlobal == 0)
   {
      mStatus = ::GetLastError();
      return;
   }

   BYTE * pStr = (BYTE *)::LockResource( hGlobal );
   if (pStr == 0)
   {
      mStatus = ::GetLastError();
      return;
   }

   // Is this text UNICODE?
   bool bUNICODE = false;

   // For starters it has to be of even length
   if ((resSz % 2) == 0)
   {
      // Better let Windows take over here
      INT flags = IS_TEXT_UNICODE_STATISTICS 
                | IS_TEXT_UNICODE_REVERSE_SIGNATURE
                | IS_TEXT_UNICODE_SIGNATURE;

      INT checkLen = (INT)resSz;
      if (checkLen > MAX_UNICODE_CHECK_LEN)
      {
         checkLen = MAX_UNICODE_CHECK_LEN;
      }

      BOOL bUNI = ::IsTextUnicode( (LPCVOID)pStr, checkLen, &flags );
      if (bUNI != FALSE)
      {
         bUNICODE = true;

         if ( (flags & IS_TEXT_UNICODE_SIGNATURE)
         ||   (flags & IS_TEXT_UNICODE_REVERSE_SIGNATURE) )
         {
            // File starts with a UNICODE BOM, skip it
            ULONG charSz = (ULONG)sizeof(USHORT);
            pStr += charSz;
            resSz -= charSz;
         }
      }
   }

#ifdef UNICODE
            
   if (bUNICODE == true)
   {
      // UNICODE item string, UNICODE core - no problem
      resSz /= (DWORD)sizeof(USHORT);
      mText = CString( (LPWSTR)pStr, resSz );
   }
   else
   {
      // ANSI item string, UNICODE core - conversion needed
      mText = CString( (LPSTR)pStr, resSz );
   }

#else
            
   if (bUNICODE == true)
   {
      // UNICODE item string, ANSI core - conversion needed
      resSz /= (DWORD)sizeof(USHORT);
      mText = CString( (LPWSTR)pStr, resSz );
   }
   else
   {
      // ANSI item string, ANSI core - no problem
      mText = CString( (LPSTR)pStr, resSz );
   }

#endif

   // Success!
   mStatus = NO_ERROR;
}

/*===========================================================================
METHOD:
   cDB2TextFile (Public Method)

DESCRIPTION:
   Construct object/copy from buffer into memory

PARAMETERS
   pBuffer     [ I ] - Buffer to copy from
   bufferLen   [ I ] - Size of above buffer

RETURN VALUE:
   None
===========================================================================*/
cDB2TextFile::cDB2TextFile(
   BYTE *                     pBuffer, 
   ULONG                      bufferLen )
   :  mText( _T("") ),
      mStatus( ERROR_FILE_NOT_FOUND ),
      mCurrentPos( 0 )
{
   // Is this text UNICODE?
   bool bUNICODE = false;

   // For starters it has to be of even length
   if ((bufferLen % 2) == 0)
   {
      // Better let Windows take over here
      INT flags = IS_TEXT_UNICODE_STATISTICS 
                | IS_TEXT_UNICODE_REVERSE_SIGNATURE
                | IS_TEXT_UNICODE_SIGNATURE;

      INT checkLen = (INT)bufferLen;
      if (checkLen > MAX_UNICODE_CHECK_LEN)
      {
         checkLen = MAX_UNICODE_CHECK_LEN;
      }

      BOOL bUNI = ::IsTextUnicode( (LPCVOID)pBuffer, checkLen, &flags );
      if (bUNI != FALSE)
      {
         bUNICODE = true;

         if ( (flags & IS_TEXT_UNICODE_SIGNATURE)
         ||   (flags & IS_TEXT_UNICODE_REVERSE_SIGNATURE) )
         {
            // File starts with a UNICODE BOM, skip it
            ULONG charSz = (ULONG)sizeof(USHORT);
            pBuffer += charSz;
            bufferLen -= charSz;
         }
      }
   }

#ifdef UNICODE
            
   if (bUNICODE == true)
   {
      // UNICODE item string, UNICODE core - no problem
      bufferLen /= (DWORD)sizeof(USHORT);
      mText = CString( (LPWSTR)pBuffer, bufferLen );
   }
   else
   {
      // ANSI item string, UNICODE core - conversion needed
      mText = CString( (LPSTR)pBuffer, bufferLen );
   }

#else
            
   if (bUNICODE == true)
   {
      // UNICODE item string, ANSI core - conversion needed
      bufferLen /= (DWORD)sizeof(USHORT);
      mText = CString( (LPWSTR)pBuffer, bufferLen );
   }
   else
   {
      // ANSI item string, ANSI core - no problem
      mText = CString( (LPSTR)pBuffer, bufferLen );
   }

#endif

   // Success!
   mStatus = NO_ERROR;
}

/*===========================================================================
METHOD:
   ~cDB2TextFile (Public Method)

DESCRIPTION:
   Destructor

RETURN VALUE:
   None
===========================================================================*/
cDB2TextFile::~cDB2TextFile()
{
   // Nothing to do
}

/*===========================================================================
METHOD:
   ReadLine (Public Method)

DESCRIPTION:
   Read the next available line

PARAMETERS
   line        [ O ] - Line (minus CR/LF)

RETURN VALUE:
   None
===========================================================================*/
bool cDB2TextFile::ReadLine( CString & line )
{
   // Assume failure
   bool bOK = false;
   if (IsValid() == false)
   {
      return bOK;
   }

   int len = mText.GetLength();
   if (mCurrentPos >= len)
   {
      return bOK;
   }

   int newIdx = mText.Find( _T('\n'), mCurrentPos );
   if (newIdx == -1)
   {
      // Possibly the end of the file
      newIdx = len;
   }

   if (newIdx < mCurrentPos)
   {
      return bOK;
   }

   if (newIdx == mCurrentPos)
   {
      // Return an empty line
      mCurrentPos++;    
      line = _T("");

      bOK = true;
   }
   else
   {
      // Grab line
      line = mText.Mid( mCurrentPos, (newIdx - mCurrentPos) );

      int lineLen = line.GetLength();
      LPTSTR pBuf = line.GetBuffer( 0 );

      // Check for CR
      if (lineLen > 0 && pBuf[lineLen - 1] == _T('\r'))
      {
         line.GetBufferSetLength( lineLen - 1 );
      }

      mCurrentPos = newIdx + 1;
      bOK = true;
   }

   return bOK;
}

