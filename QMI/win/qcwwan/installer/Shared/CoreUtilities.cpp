/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          COREUTILITIES . C

GENERAL DESCRIPTION

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

//---------------------------------------------------------------------------
// Include Files
//---------------------------------------------------------------------------
#include "Stdafx.h"
#include "CoreUtilities.h"

#include <cstring>
#include <climits>
#include <ShlObj.h>

//---------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------
#ifdef _DEBUG
   #define new DEBUG_NEW
#endif

// Format specifier states
enum eFormatState
{
   eFMT_STATE_NORMAL,      // [0] Normal state; outputting literal characters
   eFMT_STATE_PERCENT,     // [1] Just read '%'
   eFMT_STATE_FLAG,        // [2] Just read flag character
   eFMT_STATE_WIDTH,       // [3] Just read width specifier
   eFMT_STATE_DOT,         // [4] Just read '.'
   eFMT_STATE_PRECIS,      // [5] Just read precision specifier
   eFMT_STATE_SIZE,        // [6] Just read size specifier
   eFMT_STATE_TYPE,        // [7] Just read type specifier
   eFMT_STATE_INVALID,     // [8] Invalid format

   eFMT_STATES             // [9] Number of format states
};

// Format specifier character classes
enum eFormatCharClass
{
   eFMT_CH_CLASS_OTHER,    // [0] Character with no special meaning
   eFMT_CH_CLASS_PERCENT,  // [1] '%'
   eFMT_CH_CLASS_DOT,      // [2] '.'
   eFMT_CH_CLASS_STAR,     // [3] '*'
   eFMT_CH_CLASS_ZERO,     // [4] '0'
   eFMT_CH_CLASS_DIGIT,    // [5] '1'..'9'
   eFMT_CH_CLASS_FLAG,     // [6] ' ', '+', '-', '#'
   eFMT_CH_CLASS_SIZE,     // [7] 'h', 'l', 'L', 'N', 'F', 'w'
   eFMT_CH_CLASS_TYPE      // [8] Type specifying character
};

// Lookup table for determining class of a character (lower nibble) 
// and next format specifier state (upper nibble)
const UCHAR gLookupTable[] = 
{
   0x06, // ' ', FLAG
   0x80, // '!', OTHER
   0x80, // '"', OTHER
   0x86, // '#', FLAG
   0x80, // '$', OTHER
   0x81, // '%', PERCENT 
   0x80, // '&', OTHER
   0x00, // ''', OTHER
   0x00, // '(', OTHER
   0x10, // ')', OTHER
   0x03, // '*', STAR
   0x86, // '+', FLAG
   0x80, // ',', OTHER
   0x86, // '-', FLAG
   0x82, // '.', DOT
   0x80, // '/', OTHER
   0x14, // '0', ZERO
   0x05, // '1', DIGIT
   0x05, // '2', DIGIT
   0x45, // '3', DIGIT
   0x45, // '4', DIGIT
   0x45, // '5', DIGIT
   0x85, // '6', DIGIT
   0x85, // '7', DIGIT
   0x85, // '8', DIGIT
   0x05, // '9', DIGIT
   0x00, // :!', OTHER
   0x00, // ';', OTHER
   0x30, // '<', OTHER
   0x30, // '=', OTHER
   0x80, // '>', OTHER
   0x50, // '?', OTHER
   0x80, // '@', OTHER
   0x80, // 'A', OTHER
   0x00, // 'B', OTHER
   0x08, // 'C', TYPE
   0x00, // 'D', OTHER
   0x28, // 'E', TYPE
   0x27, // 'F', SIZE
   0x38, // 'G', TYPE
   0x50, // 'H', OTHER
   0x57, // 'I', SIZE
   0x80, // 'J', OTHER
   0x00, // 'K', OTHER
   0x07, // 'L', SIZE
   0x00, // 'M', OTHER
   0x37, // 'N', SIZE
   0x30, // 'O', OTHER
   0x30, // 'P', OTHER
   0x50, // 'Q', OTHER
   0x50, // 'R', OTHER
   0x88, // 'S', TYPE
   0x00, // 'T', OTHER
   0x00, // 'U', OTHER
   0x00, // 'V', OTHER
   0x20, // 'W', OTHER
   0x28, // 'X', TYPE
   0x80, // 'Y', OTHER
   0x88, // 'Z', TYPE
   0x80, // '[', OTHER
   0x80, // '\', OTHER
   0x00, // ']', OTHER
   0x00, // '^', OTHER
   0x00, // '-', OTHER
   0x60, // '`', OTHER
   0x60, // 'a', OTHER
   0x60, // 'b', OTHER
   0x68, // 'c', TYPE
   0x68, // 'd', TYPE
   0x68, // 'e', TYPE
   0x08, // 'f', TYPE
   0x08, // 'g', TYPE
   0x07, // 'h', SIZE
   0x78, // 'i', TYPE
   0x70, // 'j', OTHER
   0x70, // 'k', OTHER
   0x77, // 'l', SIZE
   0x70, // 'm', OTHER
   0x70, // 'n', OTHER
   0x08, // 'o', TYPE
   0x08, // 'p', TYPE
   0x00, // 'q', OTHER
   0x00, // 'r', OTHER
   0x08, // 's', TYPE
   0x00, // 't', OTHER
   0x08, // 'u', TYPE
   0x00, // 'v', OTHER
   0x07, // 'w', SIZE
   0x08  // 'x', TYPE
};

/*=========================================================================*/
// Free Methods
/*=========================================================================*/

/*===========================================================================
METHOD:
   IsWhitespace (Private Free Method)

DESCRIPTION:
   Is this whitespace?

PARAMETERS:   
   pStr          [ I ] - The string 

RETURN VALUE:
   bool
===========================================================================*/
static bool IsWhitespace( LPCTSTR pStr )
{
   bool bWS = false;

#ifdef UNICODE
   wint_t c = (wint_t)*pStr;
   if (iswspace( c ))
   {
      bWS = true;
   }
#else
   int c = (int)*pStr;
   if (isspace( c ))
   {
      bWS = true;
   }
#endif

   return bWS;
}

/*===========================================================================
METHOD:
   IsHexadecimalString (Private Free Method)

DESCRIPTION:
   Is this a hexadecimal digits string?

PARAMETERS:   
   pStr          [ I ] - The string 

RETURN VALUE:
   bool
===========================================================================*/
static bool IsHexadecimalString( LPCTSTR pStr )
{
   // Assume not
   bool bHex = false;

   // Skip whitespace
   LPCTSTR pTmp = pStr;
   while (IsWhitespace( pTmp ) == true)
   {
      pTmp++;
   }

   // Skip leading +/-
   TCHAR ch = *pTmp;
   if (ch == _T('+') || ch == _T('-'))
   {
      pTmp++;
   }

   if (*pTmp == _T('0'))
   {
      pTmp++;
      if (*pTmp == _T('x') || *pTmp == _T('X'))
      {
         bHex = true;
      }
   }

   return bHex;
}

/*===========================================================================
METHOD:
   IsNegativeString (Private Free Method)

DESCRIPTION:
   Is this a string starting with a negative sign?

PARAMETERS:   
   pStr          [ I ] - The string 

RETURN VALUE:
   bool
===========================================================================*/
static bool IsNegativeString( LPCTSTR pStr )
{
   // Assume not
   bool bNeg = false;

   // Skip whitespace
   LPCTSTR pTmp = pStr;
   while (IsWhitespace( pTmp ) == true)
   {
      pTmp++;
   }

   TCHAR ch = *pTmp;
   if (ch == _T('-'))
   {
      bNeg = true;
   }

   return bNeg;
}

/*===========================================================================
METHOD:
   StringToLONG (Free Method)

DESCRIPTION:
   Replacement/front end for _tcstol
  
   NOTE: _tcstol does not correctly handle a negative integer
   when specified in hexadecimal, so we have to check for that
   first

PARAMETERS:   
   pStr          [ I ] - The string 
   base          [ I ] - Base for conversion
   val           [ O ] - Resulting value

RETURN VALUE:
   bool
===========================================================================*/
bool StringToLONG( 
   LPCTSTR                    pStr,
   int                        base,
   LONG &                     val )
{
   // Assume failure
   bool bOK = false;
   if (pStr == 0 || *pStr == 0)
   {
      return bOK;
   }
  
   // Hexadecimal?
   if (base == 16 || (base == 0 && IsHexadecimalString( pStr ) == true))
   {
      // No negative hexadecimal strings allowed
      if (IsNegativeString( pStr ) == false)
      {
         // Reset error
         errno = 0;

         // Use the unsigned version, then cast
         LPTSTR pEnd = (LPTSTR)pStr;
         ULONG tmpVal = _tcstoul( pStr, &pEnd, base );
         if (tmpVal != ULONG_MAX || errno != ERANGE)
         {
            // Where did we end?
            if (pEnd != pStr && (*pEnd == 0 || IsWhitespace( pEnd ) == true))
            {
               // Success!
               val = (LONG)tmpVal;
               bOK = true;
            }
         }
      }
   }
   else
   {
      // Proceed as normal
      LPTSTR pEnd = (LPTSTR)pStr;
      LONG tmpVal = _tcstol( pStr, &pEnd, base );
      if ((tmpVal != LONG_MAX && tmpVal != LONG_MIN) || errno != ERANGE)
      {
         // Where did we end?
         if (pEnd != pStr && (*pEnd == 0 || IsWhitespace( pEnd ) == true))
         {             
            // Success!
            val = tmpVal;
            bOK = true;
         }
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   StringToULONG (Free Method)

DESCRIPTION:
   Replacement/front end for _tcstoul
  
PARAMETERS:   
   pStr          [ I ] - The string 
   base          [ I ] - Base for conversion
   val           [ O ] - Resulting value

RETURN VALUE:
   bool
===========================================================================*/
bool StringToULONG( 
   LPCTSTR                    pStr,
   int                        base,
   ULONG &                    val )
{
   // Assume failure
   bool bOK = false;
   if (pStr == 0 || *pStr == 0)
   {
      return bOK;
   }

   // No negative strings allowed
   if (IsNegativeString( pStr ) == true)
   {
      return bOK;
   }
   
   // Reset error
   errno = 0;

   LPTSTR pEnd = (LPTSTR)pStr;
   ULONG tmpVal = _tcstoul( pStr, &pEnd, base );
   if (tmpVal != ULONG_MAX || errno != ERANGE)
   {
      if (pEnd != pStr && (*pEnd == 0 || IsWhitespace( pEnd ) == true))
      {
         // Success!
         val = tmpVal;
         bOK = true;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   StringToLONGLONG (Free Method)

DESCRIPTION:
   Replacement/front end for _tcstoi64
  
   NOTE: _tcstoi64 does not correctly handle a negative integer
   when specified in hexadecimal, so we have to check for that
   first

PARAMETERS:   
   pStr          [ I ] - The string 
   base          [ I ] - Base for conversion
   val           [ O ] - Resulting value

RETURN VALUE:
   bool
===========================================================================*/
bool StringToLONGLONG( 
   LPCTSTR                    pStr,
   int                        base,
   LONGLONG &                 val )
{
   // Assume failure
   bool bOK = false;
   if (pStr == 0 || *pStr == 0)
   {
      return bOK;
   }

   if (base == 16 || (base == 0 && IsHexadecimalString( pStr ) == true))
   {
      // No negative hexadecimal strings allowed
      if (IsNegativeString( pStr ) == false)
      {
         // Reset error
         errno = 0;

         // Use the unsigned version, then cast
         LPTSTR pEnd = (LPTSTR)pStr;
         ULONGLONG tmpVal = _tcstoui64( pStr, &pEnd, base );
         if (tmpVal != _UI64_MAX || errno != ERANGE)
         {
            // Where did we end?
            if (pEnd != pStr && (*pEnd == 0 || IsWhitespace( pEnd ) == true))
            {
               // Success!
               val = (LONGLONG)tmpVal;
               bOK = true;
            }
         }
      }
   }
   else
   {
      // Proceed as normal
      LPTSTR pEnd = (LPTSTR)pStr;
      LONGLONG tmpVal = _tcstoi64( pStr, &pEnd, base );
      if ((tmpVal != _I64_MAX && tmpVal != _I64_MIN) || errno != ERANGE)
      {
         if (pEnd != pStr && (*pEnd == 0 || IsWhitespace( pEnd ) == true))
         {
            // Success!
            val = tmpVal;
            bOK = true;
         }
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   StringToULONGLONG (Free Method)

DESCRIPTION:
   Replacement/front end for _tcstoui64
  
PARAMETERS:   
   pStr          [ I ] - The string 
   base          [ I ] - Base for conversion
   val           [ O ] - Resulting value

RETURN VALUE:
   bool
===========================================================================*/
bool StringToULONGLONG( 
   LPCTSTR                    pStr,
   int                        base,
   ULONGLONG &                val )
{
   // Assume failure
   bool bOK = false;
   if (pStr == 0 || *pStr == 0)
   {
      return bOK;
   }
 
   // No negative strings allowed
   if (IsNegativeString( pStr ) == true)
   {
      return bOK;
   }
   
   // Reset error
   errno = 0;

   LPTSTR pEnd = (LPTSTR)pStr;
   ULONGLONG tmpVal = _tcstoui64( pStr, &pEnd, base );
   if (tmpVal != _UI64_MAX || errno != ERANGE)
   {
      if (pEnd != pStr && (*pEnd == 0 || IsWhitespace( pEnd ) == true))
      {
         // Success!
         val = tmpVal;
         bOK = true;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   ParseCommandLine (Free Method)

DESCRIPTION:
   Parse a command line to tokens (a command line is a string of
   space delimited values where a value that contains space is
   enclosed in text)
  
PARAMETERS:
   commandLine [ I ] - The characters separating tokens
   tokens      [ O ] - The resultant vector of tokens

RETURN VALUE:
   None
===========================================================================*/
void ParseCommandLine(
   CString                    commandLine,
   std::vector <CString> &    tokens )
{
   if (commandLine.GetLength() <= 0)
   {
      return;
   }

   TCHAR SPACE = _T(' ');
   TCHAR QUOTE = _T('\"');
   ULONG count = (ULONG)commandLine.GetLength();

   LPTSTR pTmp = commandLine.GetBuffer( commandLine.GetLength() );

   CString name = _T("");
   CString val = _T("");

   for (ULONG c = 0; c < count; c++)
   {
      // Skip leading spaces
      while (pTmp[c] == SPACE)
      {
         if (++c >= count)
         {
            commandLine.ReleaseBuffer();
            return;
         }
      }

      // In quotes?
      TCHAR nextSep = SPACE;
      if (pTmp[c] == QUOTE)
      {
         nextSep = QUOTE;
         if (++c >= count)
         {
            // Unable to find trailing quote
            tokens.clear();
            commandLine.ReleaseBuffer();
            return;
         }
      }

      // Advance to next seperator
      LPCTSTR pStart = pTmp + c;
      while (pTmp[c] != nextSep)
      {
         if (++c >= count)
         {
            if (nextSep != QUOTE)
            {
               // End of string equals end of value
               val = pStart;
               break;
            }
            else
            {
               // Unable to find trailing quote
               tokens.clear();
               commandLine.ReleaseBuffer();
               return;
            }            
         }
      }

      pTmp[c] = 0;
      if (pStart[0] == 0)
      {
         val = _T("");
      }
      else
      {
         val = pStart;
      }

      tokens.push_back( val );
   }
}

/*===========================================================================
METHOD:
   ParseTokens (Free Method)

DESCRIPTION:
   Parse a line into individual tokens

   NOTE: No attempt is made to handle accidental separators, i.e. searching
   for ',' on 'foo, bar, "foo, bar"' will return four tokens, not three so
   pick something like '^' instead of ','!
  
PARAMETERS:
   pSeparator  [ I ] - The characters separating tokens
   pLine       [ I ] - The string being parsed
   tokens      [ O ] - The resultant vector of tokens

RETURN VALUE:
   None
===========================================================================*/
void ParseTokens(
   LPCTSTR                    pSeparator,
   LPTSTR                     pLine,
   std::vector <LPTSTR> &     tokens )
{
   if (pSeparator != 0 && pSeparator[0] != 0 && pLine != 0 && pLine[0] != 0)
   {
      TCHAR * pContext = 0;
      LPTSTR pToken = _tcstok_s( pLine, pSeparator, &pContext );
      while (pToken != 0)
      {
         // Store token
         tokens.push_back( pToken );

         // Get next token:
         pToken = _tcstok_s( 0, pSeparator, &pContext );
      }
   }
}

/*===========================================================================
METHOD:
   ParseFormatSpecifier (Free Method)

DESCRIPTION:
   Parse a format specifier into individual format type tokens
  
PARAMETERS:
   pFmt        [ I ] - The format specifier (must be NULL terminated)
   fmtLen      [ I ] - Length of above format specifier
   fmtTypes    [ O ] - The individual format type tokens ('d', 'u', 'us' etc.)

RETURN VALUE:
   bool - Valid format specifier?
===========================================================================*/
bool ParseFormatSpecifier( 
   LPCTSTR                    pFmt,
   ULONG                      fmtLen,
   std::vector <TCHAR> &      fmtTypes )
{
   // Assume failure
   bool bOK = false;

   // Make sure string is NULL terminated
   TCHAR ch; 
   ULONG chars = 0;
   while (chars < fmtLen)
   {
      if (pFmt[chars] == _T('\0'))
      {
         break;
      }
      else
      {
         chars++;
      }
   }

   if (pFmt[chars] != _T('\0'))
   {
      return bOK;
   }

   // Extract individual format type tokens
   eFormatState state = eFMT_STATE_NORMAL; 
   eFormatCharClass cc = eFMT_CH_CLASS_OTHER;
   while ((ch = *pFmt++) != _T('\0') && state != eFMT_STATE_INVALID) 
   {
      // Find character class
      cc = eFMT_CH_CLASS_OTHER;
      if (ch >= _T(' ') && ch <= _T('x'))
      {
         cc = (eFormatCharClass)(gLookupTable[ch - _T(' ')] & 0xF);
      }

      // Find next state
      state = (eFormatState)(gLookupTable[cc * eFMT_STATES + (state)] >> 4);
      switch (state) 
      {
         case eFMT_STATE_NORMAL:
         NORMAL_STATE:
            break;

         case eFMT_STATE_PERCENT:
         case eFMT_STATE_FLAG:
         case eFMT_STATE_DOT:
            break;

         case eFMT_STATE_WIDTH:
         case eFMT_STATE_PRECIS:
            if (ch == _T('*')) 
            {
               fmtTypes.push_back( ch );
            }
            break;

         case eFMT_STATE_SIZE:
            switch (ch) 
            {
               case _T('l'):
                  if (*pFmt == _T('l'))
                  {
                     ++pFmt;
                  }
                  break;

               case _T('I'):
                  if ( (*pFmt == _T('6')) && (*(pFmt + 1) == _T('4')) )
                  {
                     pFmt += 2;
                  }
                  else if ( (*pFmt == _T('3')) && (*(pFmt + 1) == _T('2')) )
                  {
                     pFmt += 2;
                  }
                  else if ( (*pFmt == _T('d')) 
                        ||  (*pFmt == _T('i')) 
                        ||  (*pFmt == _T('o')) 
                        ||  (*pFmt == _T('u')) 
                        ||  (*pFmt == _T('x')) 
                        ||  (*pFmt == _T('X')) )
                  {
                     // Nothing further needed
                  }
                  else 
                  {
                     state = eFMT_STATE_NORMAL;
                     goto NORMAL_STATE;
                  }
                  break;

                  case _T('h'):
                  case _T('w'):
                     break;
               }
               break;

         case eFMT_STATE_TYPE:
            fmtTypes.push_back( ch );
            break;
      }
    }

   bOK = (state == eFMT_STATE_NORMAL || state == eFMT_STATE_TYPE);
   return bOK;
}

/*===========================================================================
METHOD:
   FromString (Free Method)

DESCRIPTION:
   Convert a string to a value
  
PARAMETERS:   
   pStr          [ I ] - The string 
   theType       [ O ] - Resulting value

RETURN VALUE:
   bool
===========================================================================*/
bool FromString( 
   LPCTSTR                    pStr,
   CHAR &                     theType )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      LONG val = LONG_MAX;
      bOK = StringToLONG( pStr, 0, val );
      if (bOK == true)
      {
         // Reset status
         bOK = false;

         // Was this provided as a hexadecimal string?
         if (IsHexadecimalString( pStr ) == true)
         {
            // Yes, the return value is a LONG, so check against
            // the maximum range for a UCHAR, before casting to
            // a CHAR (to pick sign back up)
            if (val <= UCHAR_MAX)
            {
               // Success!
               theType = (CHAR)val;
               bOK = true;
            }           
         }          
         else if (val >= SCHAR_MIN && val <= SCHAR_MAX)
         {
            // Success!
            theType = (CHAR)val;
            bOK = true;
         }
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   FromString (Free Method)

DESCRIPTION:
   Convert a string to a value
  
PARAMETERS:   
   pStr          [ I ] - The string 
   theType       [ O ] - Resulting value

RETURN VALUE:
   bool
===========================================================================*/
bool FromString( 
   LPCTSTR                    pStr,
   UCHAR &                    theType )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      ULONG val = ULONG_MAX;
      bOK = StringToULONG( pStr, 0, val );
      if (bOK == true && val <= UCHAR_MAX)
      {
         // Success!
         theType = (UCHAR)val;         
      }
      else
      {
         bOK = false;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   FromString (Free Method)

DESCRIPTION:
   Convert a string to a value
  
PARAMETERS:   
   pStr          [ I ] - The string 
   theType       [ O ] - Resulting value

RETURN VALUE:
   bool
===========================================================================*/
bool FromString( 
   LPCTSTR                    pStr,
   SHORT &                    theType )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      LONG val = LONG_MAX;
      bOK = StringToLONG( pStr, 0, val );
      if (bOK == true)
      {
         // Reset status
         bOK = false;

         // Was this provided as a hexadecimal string?
         if (IsHexadecimalString( pStr ) == true)
         {
            // Yes, the return value is a LONG, so check against
            // the maximum range for a USHORT, before casting to
            // a SHORT (to pick sign back up)
            if (val <= USHRT_MAX)
            {
               // Success!
               theType = (SHORT)val;
               bOK = true;
            }
         }
         else if (val >= SHRT_MIN && val <= SHRT_MAX)
         {
            // Success!
            theType = (SHORT)val;
            bOK = true;
         }
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   FromString (Free Method)

DESCRIPTION:
   Convert a string to a value
  
PARAMETERS:   
   pStr          [ I ] - The string 
   theType       [ O ] - Resulting value

RETURN VALUE:
   bool
===========================================================================*/
bool FromString( 
   LPCTSTR                    pStr,
   USHORT &                   theType )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      ULONG val = ULONG_MAX;
      bOK = StringToULONG( pStr, 0, val );
      if (bOK == true && val <= USHRT_MAX)
      {
         // Success!
         theType = (USHORT)val;
      }
      else
      {
         bOK = false;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   FromString (Free Method)

DESCRIPTION:
   Convert a string to a value
  
PARAMETERS:   
   pStr          [ I ] - The string 
   theType       [ O ] - Resulting value

RETURN VALUE:
   bool
===========================================================================*/
bool FromString( 
   LPCTSTR                    pStr,
   int &                      theType )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      LONG val = LONG_MAX;
      bOK = StringToLONG( pStr, 0, val );
      if (bOK == true && (val >= INT_MIN && val <= INT_MAX))
      {
         // Success!
         theType = (int)val;
      }
      else
      {
         bOK = false;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   FromString (Free Method)

DESCRIPTION:
   Convert a string to a value
  
PARAMETERS:   
   pStr          [ I ] - The string 
   theType       [ O ] - Resulting value

RETURN VALUE:
   bool
===========================================================================*/
bool FromString( 
   LPCTSTR                    pStr,
   UINT &                     theType )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      ULONG val = ULONG_MAX;
      bOK = StringToULONG( pStr, 0, val );
      if (bOK == true && val <= UINT_MAX)
      {
         // Success!
         theType = (UINT)val;
      }
      else
      {
         bOK = false;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   FromString (Free Method)

DESCRIPTION:
   Convert a string to a value
  
PARAMETERS:   
   pStr          [ I ] - The string 
   theType       [ O ] - Resulting value

RETURN VALUE:
   bool
===========================================================================*/
bool FromString( 
   LPCTSTR                    pStr,
   LONG &                     theType )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      LONG val = LONG_MAX;
      bOK = StringToLONG( pStr, 0, val );
      if (bOK == true)
      {
         // Success!
         theType = val;
      }
      else
      {
         bOK = false;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   FromString (Free Method)

DESCRIPTION:
   Convert a string to a value
  
PARAMETERS:   
   pStr          [ I ] - The string 
   theType       [ O ] - Resulting value

RETURN VALUE:
   bool
===========================================================================*/
bool FromString( 
   LPCTSTR                    pStr,
   ULONG &                    theType )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      ULONG val = ULONG_MAX;
      bOK = StringToULONG( pStr, 0, val );
      if (bOK == true)
      {
         // Success!
         theType = val;
      }
      else
      {
         bOK = false;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   FromString (Free Method)

DESCRIPTION:
   Convert a string to a value
  
PARAMETERS:   
   pStr          [ I ] - The string 
   theType       [ O ] - Resulting value

RETURN VALUE:
   bool
===========================================================================*/
bool FromString( 
   LPCTSTR                    pStr,
   LONGLONG &                 theType )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      LONGLONG val = _I64_MAX;
      bOK = StringToLONGLONG( pStr, 0, val );
      if (bOK == true)
      {
         // Success!
         theType = val;
      }
      else
      {
         bOK = false;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   FromString (Free Method)

DESCRIPTION:
   Convert a string to a value
  
PARAMETERS:   
   pStr          [ I ] - The string 
   theType       [ O ] - Resulting value

RETURN VALUE:
   bool
===========================================================================*/
bool FromString( 
   LPCTSTR                    pStr,
   ULONGLONG &                theType )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      ULONGLONG val = _UI64_MAX;
      bOK = StringToULONGLONG( pStr, 0, val );
      if (bOK == true)
      {
         // Success!
         theType = val;
      }
      else
      {
         bOK = false;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   ToString (Free Method)

DESCRIPTION:
   Convert a value to a string
  
PARAMETERS:   
   val           [ I ] - The value to convert
   pStr          [ O ] - The resulting string 

RETURN VALUE:
   bool
===========================================================================*/
bool ToString( 
   CHAR                       val,
   LPTSTR                     pStr )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      int tmp = (int)val;
      int rc = _sntprintf_s( pStr, 
                             (size_t)SUGGESTED_BUFFER_LEN,
                             (size_t)(SUGGESTED_BUFFER_LEN - 1),
                             _T("%d"), 
                             tmp ); 

      if (rc < 0)
      {
         pStr[0] = 0;
      }
      else
      {
         // Success!
         bOK = true;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   ToString (Free Method)

DESCRIPTION:
   Convert a value to a string
  
PARAMETERS:   
   val           [ I ] - The value to convert
   pStr          [ O ] - The resulting string 

RETURN VALUE:
   bool
===========================================================================*/
bool ToString( 
   UCHAR                      val,
   LPTSTR                     pStr )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      ULONG tmp = (ULONG)val;
      int rc = _sntprintf_s( pStr, 
                             (size_t)SUGGESTED_BUFFER_LEN,
                             (size_t)(SUGGESTED_BUFFER_LEN - 1),
                             _T("%u"), 
                             tmp );

      if (rc < 0)
      {
         pStr[0] = 0;
      }
      else
      {
         // Success!
         bOK = true;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   ToString (Free Method)

DESCRIPTION:
   Convert a value to a string
  
PARAMETERS:   
   val           [ I ] - The value to convert
   pStr          [ O ] - The resulting string 

RETURN VALUE:
   bool
===========================================================================*/
bool ToString( 
   SHORT                      val,
   LPTSTR                     pStr )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      int tmp = (int)val;
      int rc = _sntprintf_s( pStr, 
                             (size_t)SUGGESTED_BUFFER_LEN, 
                             (size_t)(SUGGESTED_BUFFER_LEN - 1),
                             _T("%d"), 
                             tmp ); 

      if (rc < 0)
      {
         pStr[0] = 0;
      }
      else
      {
         // Success!
         bOK = true;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   ToString (Free Method)

DESCRIPTION:
   Convert a value to a string
  
PARAMETERS:   
   val           [ I ] - The value to convert
   pStr          [ O ] - The resulting string 

RETURN VALUE:
   bool
===========================================================================*/
bool ToString( 
   USHORT                     val,
   LPTSTR                     pStr )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      ULONG tmp = (ULONG)val;
      int rc = _sntprintf_s( pStr, 
                             (size_t)SUGGESTED_BUFFER_LEN, 
                             (size_t)(SUGGESTED_BUFFER_LEN - 1),
                             _T("%u"), 
                             tmp );

      if (rc < 0)
      {
         pStr[0] = 0;
      }
      else
      {
         // Success!
         bOK = true;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   ToString (Free Method)

DESCRIPTION:
   Convert a value to a string
  
PARAMETERS:   
   val           [ I ] - The value to convert
   pStr          [ O ] - The resulting string 

RETURN VALUE:
   bool
===========================================================================*/
bool ToString( 
   int                        val,
   LPTSTR                     pStr )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      int rc = _sntprintf_s( pStr, 
                             (size_t)SUGGESTED_BUFFER_LEN, 
                             (size_t)(SUGGESTED_BUFFER_LEN - 1),
                             _T("%d"), 
                             val ); 

      if (rc < 0)
      {
         pStr[0] = 0;
      }
      else
      {
         // Success!
         bOK = true;
      }
   }

   return bOK;
}


/*===========================================================================
METHOD:
   ToString (Free Method)

DESCRIPTION:
   Convert a value to a string
  
PARAMETERS:   
   val           [ I ] - The value to convert
   pStr          [ O ] - The resulting string 

RETURN VALUE:
   bool
===========================================================================*/
bool ToString( 
   UINT                       val,
   LPTSTR                     pStr )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      int rc = _sntprintf_s( pStr, 
                             (size_t)SUGGESTED_BUFFER_LEN, 
                             (size_t)(SUGGESTED_BUFFER_LEN - 1),
                             _T("%u"), 
                             val );

      if (rc < 0)
      {
         pStr[0] = 0;
      }
      else
      {
         // Success!
         bOK = true;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   ToString (Free Method)

DESCRIPTION:
   Convert a value to a string
  
PARAMETERS:   
   val           [ I ] - The value to convert
   pStr          [ O ] - The resulting string 

RETURN VALUE:
   bool
===========================================================================*/
bool ToString( 
   LONG                       val,
   LPTSTR                     pStr )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      int rc = _sntprintf_s( pStr, 
                             (size_t)SUGGESTED_BUFFER_LEN, 
                             (size_t)(SUGGESTED_BUFFER_LEN - 1),
                             _T("%d"), 
                             val ); 

      if (rc < 0)
      {
         pStr[0] = 0;
      }
      else
      {
         // Success!
         bOK = true;
      }
   }

   return bOK;
}


/*===========================================================================
METHOD:
   ToString (Free Method)

DESCRIPTION:
   Convert a value to a string
  
PARAMETERS:   
   val           [ I ] - The value to convert
   pStr          [ O ] - The resulting string 

RETURN VALUE:
   bool
===========================================================================*/
bool ToString( 
   ULONG                      val,
   LPTSTR                     pStr )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      int rc = _sntprintf_s( pStr, 
                             (size_t)SUGGESTED_BUFFER_LEN, 
                             (size_t)(SUGGESTED_BUFFER_LEN - 1),
                             _T("%u"), 
                             val );

      if (rc < 0)
      {
         pStr[0] = 0;
      }
      else
      {
         // Success!
         bOK = true;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   ToString (Free Method)

DESCRIPTION:
   Convert a value to a string
  
PARAMETERS:   
   val           [ I ] - The value to convert
   pStr          [ O ] - The resulting string 

RETURN VALUE:
   bool
===========================================================================*/
bool ToString( 
   LONGLONG                   val,
   LPTSTR                     pStr )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      int rc = _sntprintf_s( pStr, 
                             (size_t)SUGGESTED_BUFFER_LEN, 
                             (size_t)(SUGGESTED_BUFFER_LEN - 1),
                             _T("%I64d"), 
                             val ); 

      if (rc < 0)
      {
         pStr[0] = 0;
      }
      else
      {
         // Success!
         bOK = true;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   ToString (Free Method)

DESCRIPTION:
   Convert a value to a string
  
PARAMETERS:   
   val           [ I ] - The value to convert
   pStr          [ O ] - The resulting string 

RETURN VALUE:
   bool
===========================================================================*/
bool ToString( 
   ULONGLONG                  val,
   LPTSTR                     pStr )
{
   // Assume failure
   bool bOK = false;

   if (pStr != 0)
   {
      int rc = _sntprintf_s( pStr, 
                             (size_t)SUGGESTED_BUFFER_LEN,
                             (size_t)(SUGGESTED_BUFFER_LEN - 1),
                             _T("%I64u"), 
                             val );

      if (rc < 0)
      {
         pStr[0] = 0;
      }
      else
      {
         // Success!
         bOK = true;
      }
   }

   return bOK;
}

/*===========================================================================
METHOD:
   StartHRTiming (Free Method)

DESCRIPTION:
   Start a high resolution timing session
  
PARAMETERS:   
   startTime     [ O ] - The start time (HR frequency dependant)

RETURN VALUE:
   None
===========================================================================*/
void StartHRTiming( ULONGLONG & startTime )
{
   // Get starting counter
   LARGE_INTEGER countStart = { 0, 0 };
   QueryPerformanceCounter( &countStart );

   if (countStart.QuadPart > 0)
   {
      startTime = (ULONGLONG)countStart.QuadPart;
   }
   else
   {
      startTime = 0;
   }
}

/*===========================================================================
METHOD:
   StopHRTiming (Free Method)

DESCRIPTION:
   Stop a high resolution timing session
  
PARAMETERS:   
   startTime     [ I ] - The start time (HR frequency dependant)

RETURN VALUE:
   The amount of elapsed microseconds (0 upon error)
===========================================================================*/
ULONGLONG StopHRTiming( ULONGLONG & startTime )
{
   ULONGLONG elapsed = 0;   

   // Get ending counter
   LARGE_INTEGER countEnd = { 0, 0 };
   QueryPerformanceCounter( &countEnd );

   // Get elapsed counts
   if (countEnd.QuadPart > 0 && (ULONGLONG)countEnd.QuadPart > startTime)
   {
      elapsed = (ULONGLONG)countEnd.QuadPart - startTime;

      // Get hi-res timer frequency
      LARGE_INTEGER countsPerSecond = { 0, 0 };
      BOOL bHiRes = ::QueryPerformanceFrequency( &countsPerSecond );

      // If we have a hi-res timer compute the elapsed microseconds
      if (bHiRes != FALSE && countsPerSecond.QuadPart > 0)
      {
         // Convert frequency to microsecs
         countsPerSecond.QuadPart /= 1000000i64;

         // Convert elapsed count to microsecs
         elapsed /= countsPerSecond.QuadPart;
      }
   }

   return elapsed;
}

/*===========================================================================
METHOD:
   GetApplicationPath (Public Method)

DESCRIPTION:
   Return the application (or DLL) path

PARAMETERS:   
   hInstance   [ I ] - The module instance (only used to distinguish a
                       DLL from the host application)

RETURN VALUE:
   CString
===========================================================================*/
CString GetApplicationPath( HMODULE hInstance )
{
   CString tmp = _T(".\\");

   // Get directory application was launched from
   TCHAR path[MAX_PATH * 2 + 1];
   DWORD dirLen = ::GetModuleFileName( hInstance, &path[0], MAX_PATH * 2 );
   if (dirLen <= 0)
   {
      return tmp;
   }

   // Break apart into drive, folders, and module name
   std::vector <LPTSTR> tokens;
   ParseTokens( _T("\\"), path, tokens );

   UINT t;
   UINT toks = (UINT)tokens.size();
   if (toks < 2)
   {
      return tmp;
   }

   // Build up base path (omit module name)
   tmp.Empty();
   for (t = 0; t < toks - 1; t++)
   {
      tmp += tokens[t];
      tmp += _T("\\");
   }

   return tmp;
}

/*===========================================================================
METHOD:
   GetDataPath (Public Method)

DESCRIPTION:
   Return the special folder used for storing application data

RETURN VALUE:
   CString
===========================================================================*/
CString GetDataPath()
{
   CString dataPath = _T(".\\");

   // Get directory application was launched from
   TCHAR path[MAX_PATH * 2 + 1];
   path[0] = 0;

   HRESULT hr = ::SHGetFolderPath( 0,
                                   CSIDL_COMMON_APPDATA,
                                   0,
                                   SHGFP_TYPE_CURRENT,
                                   path );

   if (SUCCEEDED( hr ) && path[0] != 0)
   {
      CString tmp = path;
      tmp.Trim();

      INT len = tmp.GetLength();
      if (len > 0)
      {
         if (tmp[len -1 ] != _T('\\'))
         {
            tmp += _T('\\');
         }

         // Add in company name
         tmp += _T("Qualcomm\\");
   
         // ... and store result
         dataPath = tmp;
      }
   }

   return dataPath;
}

/*===========================================================================
METHOD:
   GetProgramPath (Public Method)

DESCRIPTION:
   Return the special folder used for storing program files

RETURN VALUE:
   CString
===========================================================================*/
CString GetProgramPath()
{
   CString programPath = _T(".\\");

   // Get directory application was launched from
   TCHAR path[MAX_PATH * 2 + 1];
   path[0] = 0;

   HRESULT hr = ::SHGetFolderPath( 0,
                                   CSIDL_PROGRAM_FILES,
                                   0,
                                   SHGFP_TYPE_CURRENT,
                                   path );

   if (SUCCEEDED( hr ) && path[0] != 0)
   {
      CString tmp = path;
      tmp.Trim();

      INT len = tmp.GetLength();
      if (len > 0)
      {
         if (tmp[len -1 ] != _T('\\'))
         {
            tmp += _T('\\');
         }

         // Add in company name
         tmp += _T("Qualcomm\\");
   
         // ... and store result
         programPath = tmp;
      }
   }

   return programPath;
}
