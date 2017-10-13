/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          COREUTILITIES . C

GENERAL DESCRIPTION

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

//---------------------------------------------------------------------------
// Pragmas
//---------------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------------
// Include Files
//---------------------------------------------------------------------------
#include <vector>
#include <set>

//---------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------

// Suggested size of an argument buffer to ToString() for any key, each
// ToString() should not write more than this size to the passed in buffer
const ULONG SUGGESTED_BUFFER_LEN = 64;

/*=========================================================================*/
// Prototypes
/*=========================================================================*/

// Replacement/front end for _tcstol
bool StringToLONG( 
   LPCTSTR                    pStr,
   int                        base,
   LONG &                     val );

// Replacement/front end for _tcstoul
bool StringToULONG( 
   LPCTSTR                    pStr,
   int                        base,
   ULONG &                    val );

// Replacement/front end for _tcstoi64
bool StringToLONGLONG( 
   LPCTSTR                    pStr,
   int                        base,
   LONGLONG &                 val );

// Replacement/front end for _tcstoui64
bool StringToULONGLONG( 
   LPCTSTR                    pStr,
   int                        base,
   ULONGLONG &                val );

// Parse a command line to tokens (a command line is a string of
// space delimited values where a value that contains space is
// enclosed in text)
void ParseCommandLine(
   CString                    commandLine,
   std::vector <CString> &    tokens );

// Parse a line into individual tokens
void ParseTokens(
   LPCTSTR                    pSeparator,
   LPTSTR                     pLine,
   std::vector <LPTSTR> &     tokens );

// Parse a format specifier into individual format type tokens
bool ParseFormatSpecifier( 
   LPCTSTR                    pFmt,
   ULONG                      fmtLen,
   std::vector <TCHAR> &      fmtTypes );

// Convert a string to a value
bool FromString( 
   LPCTSTR                    pStr,
   CHAR &                     theType );

// Convert a string to a value
bool FromString( 
   LPCTSTR                    pStr,
   UCHAR &                    theType );

// Convert a string to a value
bool FromString( 
   LPCTSTR                    pStr,
   SHORT &                    theType );

// Convert a string to a value
bool FromString( 
   LPCTSTR                    pStr,
   USHORT &                   theType );

// Convert a string to a value
bool FromString( 
   LPCTSTR                    pStr,
   int &                      theType );

// Convert a string to a value
bool FromString( 
   LPCTSTR                    pStr,
   UINT &                     theType );

// Convert a string to a value
bool FromString( 
   LPCTSTR                    pStr,
   LONG &                     theType );

// Convert a string to a value
bool FromString( 
   LPCTSTR                    pStr,
   ULONG &                    theType );

// Convert a string to a value
bool FromString( 
   LPCTSTR                    pStr,
   LONGLONG &                 theType );

// Convert a string to a value
bool FromString( 
   LPCTSTR                    pStr,
   ULONGLONG &                theType );


// Convert a value to a string
bool ToString( 
   CHAR                       val,
   LPTSTR                     pStr );

// Convert a value to a string
bool ToString( 
   UCHAR                      val,
   LPTSTR                     pStr );

// Convert a value to a string
bool ToString( 
   SHORT                      val,
   LPTSTR                     pStr );

// Convert a value to a string
bool ToString( 
   USHORT                     val,
   LPTSTR                     pStr );

// Convert a value to a string
bool ToString( 
   int                        val,
   LPTSTR                     pStr );

// Convert a value to a string
bool ToString( 
   UINT                       val,
   LPTSTR                     pStr );

// Convert a value to a string
bool ToString( 
   LONG                       val,
   LPTSTR                     pStr );

// Convert a value to a string
bool ToString( 
   ULONG                      val,
   LPTSTR                     pStr );

// Convert a value to a string
bool ToString( 
   LONGLONG                   val,
   LPTSTR                     pStr );

// Convert a value to a string
bool ToString( 
   ULONGLONG                  val,
   LPTSTR                     pStr );

// Start a high resolution timing session
void StartHRTiming( ULONGLONG & startTime );

// Stop a high resolution timing session
ULONGLONG StopHRTiming( ULONGLONG & startTime );

// Return the application (or DLL) path
CString GetApplicationPath( HMODULE hInstance = 0 );

// Return the special folder used for storing application data
CString GetDataPath();

// Return the special folder used for storing program files
CString GetProgramPath();

/*=========================================================================*/
// Free Methods
/*=========================================================================*/

/*===========================================================================
METHOD:
   ContainerToCSVString (Free Method)

DESCRIPTION:
   Convert the contents of a container to a CSV string

   NOTE: ToString() must be defined for the container value type
  
PARAMETERS:   
   cont        [ I ] - The container
   sep         [ I ] - The character separating tokens
   csv         [ O ] - The resulting comma separated string

RETURN VALUE:
   None
===========================================================================*/
template <class Container>
void ContainerToCSVString( 
   const Container &          cont,
   TCHAR                      sep,
   CString &                  csv )
{
   csv = _T("");
   if ((ULONG)cont.size() > (ULONG)0)
   {
      TCHAR keyBuf[SUGGESTED_BUFFER_LEN];

      Container::const_iterator pIter = cont.begin();
      while (pIter != cont.end())
      {
         const Container::value_type & theKey = *pIter;
         bool bOK = ToString( theKey, &keyBuf[0] );

         if (bOK == true && keyBuf[0] != 0)
         {
            if (pIter != cont.begin())
            {
               csv += sep;
            }

            csv += (LPCTSTR)&keyBuf[0];
         }

         pIter++;
      }
   }
}

/*===========================================================================
METHOD:
   CSVStringToContainer (Free Method)

DESCRIPTION:
   Populate a container from a parsed CSV string

   NOTE: FromString() must be defined for the container value type 
   NOTE: The container is emptied before this operation is attempted

PARAMETERS:   
   pSeparator  [ I ] - The characters separating tokens
   pCSV        [ I ] - The comma separated string (will be modified)
   cont        [ O ] - The resulting container
   bClear      [ I ] - Clear the container first?  NOTE: if the container
                       is not cleared first then insertions may fail for
                       duplicate keys

RETURN VALUE:
   None
===========================================================================*/
template <class Container>
void CSVStringToContainer( 
   LPCTSTR                    pSeparator, 
   LPTSTR                     pCSV,
   Container &                cont,
   bool                       bClear = true )
{
   if (pCSV != 0 && *pCSV != 0)
   {
      // Remove a leading quote?
      if (*pCSV == _T('\"'))
      {
         pCSV++;
      }

      // Remove a trailing quote?
      ULONG len = (ULONG)_tcsclen( pCSV );
      if (len > 0)
      {
         if (pCSV[len - 1] == _T('\"'))
         {
            pCSV[len - 1] = 0;
         }
      }

      // Clear the container first?
      if (bClear == true)
      {
         cont.clear();
      }

      std::vector <LPTSTR> tokens;
      ParseTokens( pSeparator, pCSV, tokens );

      std::vector <LPTSTR>::const_iterator pIter = tokens.begin();
      while (pIter != tokens.end())
      {
         LPCTSTR pTok  = *pIter;
  
         Container::value_type theKey;
         bool bOK = ::FromString( pTok, theKey );

         if (bOK == true)
         {      
            std::insert_iterator <Container> is( cont, cont.end() );
            *is = theKey;
         }
       
         pIter++;
      }
   }
}

/*===========================================================================
METHOD:
   CSVStringToValidatedContainer (Free Method)

DESCRIPTION:
   Populate a container from a parsed CSV string

   NOTE: FromString() and IsValid() must be defined for the container 
         value type (the later need not do anything but return true)

   NOTE: The container is emptied before this operation is attempted
  
PARAMETERS:   
   pSeparator  [ I ] - The characters separating tokens
   pCSV        [ I ] - The comma separated string (will be modified)
   cont        [ O ] - The resulting container

RETURN VALUE:
   None
===========================================================================*/
template <class Container>
void CSVStringToValidatedContainer( 
   LPCTSTR                    pSeparator, 
   LPTSTR                     pCSV,
   Container &                cont )
{
   cont.clear();
   if (pCSV != 0 && *pCSV != 0)
   {
      // Remove a leading quote?
      if (*pCSV == _T('\"'))
      {
         pCSV++;
      }

      // Remove a trailing quote?
      ULONG len = (ULONG)_tcsclen( pCSV );
      if (len > 0)
      {
         if (pCSV[len - 1] == _T('\"'))
         {
            pCSV[len - 1] = 0;
         }
      }

      cont.clear();

      std::vector <LPTSTR> tokens;
      ParseTokens( pSeparator, pCSV, tokens );

      std::vector <LPTSTR>::const_iterator pIter = tokens.begin();
      while (pIter != tokens.end())
      {
         LPCTSTR pTok  = *pIter;

         Container::value_type theKey;
         bool bOK = ::FromString( pTok, theKey );

         if (bOK == true)
         {
            bool bValid = IsValid( theKey );
            if (bValid == true)
            {
               std::insert_iterator <Container> is( cont, cont.end() );
               *is = theKey;
            }
         }
      
         pIter++;
      }
   }
}

/*===========================================================================
METHOD:
   SetDiff (Free Method)

DESCRIPTION:
   Given two sets return a third that contains everything in the first
   set but not the second set

PARAMETERS:
   setA        [ I ] - The first set
   setB        [ I ] - The second set
  
RETURN VALUE:
   std::set <T> - the difference
===========================================================================*/
template <class T>
std::set <T> SetDiff(
   const std::set <T> &       setA,
   const std::set <T> &       setB )
{
   std::set <T> retSet;

   if (setB.size() == 0)
   {
      // Everything that is in the first set but not the second
      // (empty) set is ... the first set!
      retSet = setA;
   }
   else if (setA.size() == 0)
   {
      // The first set is empty, hence the return set is empty
   }
   else
   {
      // Both sets have elements, therefore the iterators will
      // be valid and we can use the standard approach
      std::set <T>::const_iterator pIterA = setA.begin();
      std::set <T>::const_iterator pIterB = setB.begin();

      for ( ; pIterA != setA.end() && pIterB != setB.end(); )
      {
         if (*pIterA < *pIterB)
         {
            retSet.insert( *pIterA );
            pIterA++;
         }
         else if (*pIterB < *pIterA)
         {
            pIterB++;
         }
         else
         {
            pIterA++;
            pIterB++;
         }
      }

      while (pIterA != setA.end())
      {
         retSet.insert( *pIterA );
         pIterA++;
      }
   }

   return retSet;
}

/*===========================================================================
METHOD:
   SetIntersection (Free Method)

DESCRIPTION:
   Given two sets return a third that contains everything that is in both
   sets

PARAMETERS:
   setA        [ I ] - The first set
   setB        [ I ] - The second set
  
RETURN VALUE:
   std::set <T> - the union
===========================================================================*/
template <class T>
std::set <T> SetIntersection(
   const std::set <T> &       setA,
   const std::set <T> &       setB )
{
   std::set <T> retSet;

   // Neither set can be empty
   if (setA.size() != 0 && setA.size() != 0)
   {
      // Both sets have elements, therefore the iterators will
      // be valid and we can use the standard approach
      std::set <T>::const_iterator pIterA = setA.begin();
      std::set <T>::const_iterator pIterB = setB.begin();

      for ( ; pIterA != setA.end() && pIterB != setB.end(); )
      {
         if (*pIterA < *pIterB)
         {
            pIterA++;
         }
         else if (*pIterB < *pIterA)
         {
            pIterB++;
         }
         else
         {
            retSet.insert( *pIterA );
            pIterA++;
            pIterB++;
         }
      }
   }

   return retSet;
}

/*===========================================================================
METHOD:
   SetUnion (Free Method)

DESCRIPTION:
   Given two sets return a third that contains everything that is either
   in the first set or in the second set

PARAMETERS:
   setA        [ I ] - The first set
   setB        [ I ] - The second set
  
RETURN VALUE:
   std::set <T> - the union
===========================================================================*/
template <class T>
std::set <T> SetUnion(
   const std::set <T> &       setA,
   const std::set <T> &       setB )
{
   std::set <T> retSet;

   if (setB.size() == 0)
   {
      // Everything that is in the first (possibly empty) set or in 
      // the second (empty) set is ... the first set!
      retSet = setA;
   }
   else if (setA.size() == 0)
   {
      // Everything that is in the first (empty) set or in the 
      // second (possibly empty) set is ... the second set!
      retSet = setB;
   }
   else
   {
      // Both sets have elements, therefore the iterators will
      // be valid and we can use the standard approach
      std::set <T>::const_iterator pIterA = setA.begin();
      std::set <T>::const_iterator pIterB = setB.begin();

      for ( ; pIterA != setA.end() && pIterB != setB.end(); )
      {
         if (*pIterA < *pIterB)
         {
            retSet.insert( *pIterA );
            pIterA++;
         }
         else if (*pIterB < *pIterA)
         {
            retSet.insert( *pIterB );
            pIterB++;
         }
         else
         {
            retSet.insert( *pIterA );
            pIterA++;
            pIterB++;
         }
      }

      while (pIterA != setA.end())
      {
         retSet.insert( *pIterA );
         pIterA++;
      }

      while (pIterB != setB.end())
      {
         retSet.insert( *pIterB );
         pIterB++;
      }
   }

   return retSet;
}

