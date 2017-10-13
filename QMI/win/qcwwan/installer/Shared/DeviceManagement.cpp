/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          DEVICEMANAGEMENT . C

GENERAL DESCRIPTION

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
//---------------------------------------------------------------------------
// Include Files
//---------------------------------------------------------------------------
#include "Stdafx.h"
#include "DeviceManagement.h"
#include "infdev.h"
#include "WinSvc.h"
#include "string.h"
#define INITGUID
#include "GuidDef.h"
//#define AUTO_CLOSE_UNSIGNED_WARN 0
#if AUTO_CLOSE_UNSIGNED_WARN	  
#include "Wizard.h"
#endif
#include <vector>

//---------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------
#ifdef _DEBUG
   #define new DEBUG_NEW
#endif

#if AUTO_CLOSE_UNSIGNED_WARN	  
extern HMODULE g_hLib;
#endif
extern int restartRequired;

// Device class GUIDs
DEFINE_GUID( MY_GUID_PORT, 0x86E0D1E0L, 0x8089, 0x11D0, 0x9C, 0xE4, 0x08, 0x00, 0x3E, 0x30, 0x1F, 0x73 );
DEFINE_GUID( MY_GUID_MODEM, 0x2C7089AAL, 0x2E0E, 0x11D1, 0xB1, 0x14, 0x00, 0xC0, 0x4F, 0xC2, 0xAA, 0xE4 );
DEFINE_GUID( MY_GUID_NET, 0xAD498944L, 0x762F, 0x11D0, 0x8D, 0xCB, 0x00, 0xC0, 0x4F, 0xC3, 0x35, 0x8C );
DEFINE_GUID( MY_GUID_USB_HUB, 0xF18A0E88, 0xC30C, 0x11D0, 0x88, 0x15, 0x00, 0xA0, 0xC9, 0x06, 0xBE, 0xD8 );
DEFINE_GUID( MY_GUID_USB_DEV, 0xA5DCBF10L, 0x6530, 0x11D2, 0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED );

// Installer driver folder (per-OEM) names
LPCWSTR DEVICE_OEM_GENERIC   = L"generic";
LPCWSTR DEVICE_OEM_HP        = L"hp";
LPCWSTR DEVICE_OEM_LENOVO    = L"lenovo";
LPCWSTR DEVICE_OEM_ACER      = L"ad";
LPCWSTR DEVICE_OEM_SONY      = L"sony";
LPCWSTR DEVICE_OEM_TG        = L"topglobal";
LPCWSTR DEVICE_OEM_TOSHIBA   = L"toshiba";
LPCWSTR DEVICE_OEM_SAMSUNG   = L"samsung";
LPCWSTR DEVICE_OEM_GD        = L"generaldynamics";
LPCWSTR DEVICE_OEM_ASUS      = L"asus";
LPCWSTR DEVICE_OEM_SIERRA    = L"sierra";
LPCWSTR DEVICE_OEM_DIGI      = L"digi";
LPCWSTR DEVICE_OEM_DELL      = L"dell";
LPCWSTR DEVICE_OEM_PANASONIC = L"panasonic";
LPCWSTR DEVICE_OEM_OPTION    = L"option";
LPCWSTR DEVICE_OEM_QUALCOMM    = L"qualcomm";


std::set <CString> FilterList;

/*=========================================================================*/
// cDeviceManager Methods
/*=========================================================================*/

/*===========================================================================
METHOD:
   cDeviceManager (Public Method)

DESCRIPTION:
   Constructor
  
PARAMETERS:
   pDeviceOEM  [ I ] - One of the above DEVICE_OEM_* constants, used to
                       populate device ID (VID/PID) sets

   logger      [ I ] - Output log object 

RETURN VALUE:
   None
===========================================================================*/
cDeviceManager::cDeviceManager(
   LPCWSTR                    pDeviceOEM,
   cDeviceManagerLogger &     logger )
   :  mLog( logger ),
      mpDriverOEM( 0 ),
      mDriverTLA( L"" ),
      mTCPWindowsSize( 0 ),
      mTCPMaxWindowsSize( 0 ),
      mTCP123Opts( 0 ),
      mTCPMaxConnectRetransmissions( 0 ),
      mTCPSackOpts( 0 ),
      mTCPMaxDupAcks( 0 ),
      mhDLL( 0 ),
      mpFnPreinstall( 0 ),
      mpFnInstall( 0 ),
      mpFnUninstall( 0 ),
      mQCSerialPath( L"" ),
      mQCModemPath( L"" ),
      mQCNetPath( L"" ),
      mQCAlsoDeletePath(L""),
      mQCFilterPath( L"" ),
	  mQCQdssPath( L"" )
{
   CString devOEM = L"";
   if (pDeviceOEM != 0 && pDeviceOEM[0] != 0)
   {
      devOEM = pDeviceOEM;
      devOEM.MakeLower();
      devOEM.Trim();
   }

   if (devOEM.Find( DEVICE_OEM_GENERIC ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9208" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_920b" ) );

      // The Linux only iRex devices
      // mDeviceIDs.insert( CString( L"vid_05c6&pid_9274" ) );
      // mDeviceIDs.insert( CString( L"vid_05c6&pid_9275" ) );

      mpDriverOEM = DEVICE_OEM_GENERIC;
      mDriverTLA = L"";

      mTCPWindowsSize = 131072;
   }
   else if (devOEM.Find( DEVICE_OEM_HP ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_03f0&pid_241d" ) );
      mDeviceIDs.insert( CString( L"vid_03f0&pid_251d" ) );
      mpDriverOEM = DEVICE_OEM_HP;
      mDriverTLA = L"hp";

      mTCPWindowsSize = 131072;
      mTCPMaxWindowsSize = 131072;
      mTCP123Opts = 0;
      mTCPMaxConnectRetransmissions = 0;
   }
   else if (devOEM.Find( DEVICE_OEM_LENOVO ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9204" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9205" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9284" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9285" ) );
      mpDriverOEM = DEVICE_OEM_LENOVO;
      mDriverTLA = L"lno";

      mTCPWindowsSize = 260176;
   }
   else if (devOEM.Find( DEVICE_OEM_ACER ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9214" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9215" ) );
      mpDriverOEM = DEVICE_OEM_ACER;
      mDriverTLA = L"ad";
   }
   else if (devOEM.Find( DEVICE_OEM_SONY ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9224" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9225" ) );
      mpDriverOEM = DEVICE_OEM_SONY;
      mDriverTLA = L"sny";
   }
   else if (devOEM.Find( DEVICE_OEM_TG ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9234" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9235" ) );
      mpDriverOEM = DEVICE_OEM_TG;
      mDriverTLA = L"tg";
   }
   else if (devOEM.Find( DEVICE_OEM_TOSHIBA ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_0930&pid_130f" ) );
      mDeviceIDs.insert( CString( L"vid_0930&pid_1310" ) );
      mpDriverOEM = DEVICE_OEM_TOSHIBA;
      mDriverTLA = L"tsh";

      mTCPWindowsSize = 146000;
      mTCPMaxWindowsSize = 146000;
      mTCP123Opts = 3;
      mTCPMaxConnectRetransmissions = 5;
   }
   else if (devOEM.Find( DEVICE_OEM_SAMSUNG ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9244" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9245" ) );
      mpDriverOEM = DEVICE_OEM_SAMSUNG;
      mDriverTLA = L"ssg";
   }
   else if (devOEM.Find( DEVICE_OEM_GD ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9254" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9255" ) );
      mpDriverOEM = DEVICE_OEM_GD;
      mDriverTLA = L"gd";
   }
   else if (devOEM.Find( DEVICE_OEM_ASUS ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9264" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9265" ) );
      mpDriverOEM = DEVICE_OEM_ASUS;
      mDriverTLA = L"asu";
   }
   else if (devOEM.Find( DEVICE_OEM_SIERRA ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_1199&pid_9000" ) );
      mDeviceIDs.insert( CString( L"vid_1199&pid_9001" ) );
      mDeviceIDs.insert( CString( L"vid_1199&pid_9002" ) );
      mDeviceIDs.insert( CString( L"vid_1199&pid_9003" ) );
      mDeviceIDs.insert( CString( L"vid_1199&pid_9004" ) );
      mDeviceIDs.insert( CString( L"vid_1199&pid_9005" ) );
      mDeviceIDs.insert( CString( L"vid_1199&pid_9006" ) );
      mDeviceIDs.insert( CString( L"vid_1199&pid_9007" ) );
      mDeviceIDs.insert( CString( L"vid_1199&pid_9008" ) );
      mDeviceIDs.insert( CString( L"vid_1199&pid_9009" ) );
      mDeviceIDs.insert( CString( L"vid_1199&pid_900a" ) );
      mpDriverOEM = DEVICE_OEM_SIERRA;
      mDriverTLA = L"sra";

      mTCPWindowsSize = 128480;
      mTCPMaxWindowsSize = 128480;
      mTCP123Opts = 3;
      mTCPMaxConnectRetransmissions = 0;
      mTCPSackOpts = 1;
      mTCPMaxDupAcks = 2;
   }
   else if (devOEM.Find( DEVICE_OEM_DIGI ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_04d0&pid_0720" ) );
      mDeviceIDs.insert( CString( L"vid_04d0&pid_0721" ) );
      mpDriverOEM = DEVICE_OEM_DIGI;
      mDriverTLA = L"dgi";
   }
   else if (devOEM.Find( DEVICE_OEM_DELL ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_413c&pid_8185" ) );
      mDeviceIDs.insert( CString( L"vid_413c&pid_8186" ) );
      mpDriverOEM = DEVICE_OEM_DELL;
      mDriverTLA = L"dl";
   }
   else if (devOEM.Find( DEVICE_OEM_PANASONIC ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_04da&pid_250e" ) );
      mDeviceIDs.insert( CString( L"vid_04da&pid_250f" ) );
      mpDriverOEM = DEVICE_OEM_PANASONIC;
      mDriverTLA = L"ps";

      mTCPWindowsSize = 131072;
   }
   else if (devOEM.Find( DEVICE_OEM_OPTION ) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_0af0&pid_8109" ) );
      mDeviceIDs.insert( CString( L"vid_0af0&pid_8110" ) );
      mDeviceIDs.insert( CString( L"vid_0af0&pid_8111" ) );
      mDeviceIDs.insert( CString( L"vid_0af0&pid_8112" ) );
      mDeviceIDs.insert( CString( L"vid_0af0&pid_8113" ) );
      mDeviceIDs.insert( CString( L"vid_0af0&pid_8114" ) );
      mDeviceIDs.insert( CString( L"vid_0af0&pid_8115" ) );
      mDeviceIDs.insert( CString( L"vid_0af0&pid_8116" ) );
      mDeviceIDs.insert( CString( L"vid_0af0&pid_8117" ) );
      mDeviceIDs.insert( CString( L"vid_0af0&pid_8118" ) );
      mDeviceIDs.insert( CString( L"vid_0af0&pid_8119" ) );
      mpDriverOEM = DEVICE_OEM_OPTION;
      mDriverTLA = L"opt";
   }
   else if (devOEM.Find( DEVICE_OEM_QUALCOMM) == 0)
   {
      mDeviceIDs.insert( CString( L"vid_05c6&pid_3100" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_3196" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_8000" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_8001" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_8002" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_3200" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_3197" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_6000" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_7000" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_7001" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_7002" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_7101" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_7102" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_8000" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_8001" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_8002" ) );   
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9000" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9001" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9002" ) );   
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9003" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9004" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9005" ) ); 
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9006" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9008" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9009" ) ); 
      mDeviceIDs.insert( CString( L"vid_05c6&pid_900a" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_900b" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_900c" ) ); 
      mDeviceIDs.insert( CString( L"vid_05c6&pid_900d" ) ); 
      mDeviceIDs.insert( CString( L"vid_05c6&pid_900e" ) ); 
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9012" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9013" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9016" ) );   
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9017" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9018" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9019" ) ); 
      mDeviceIDs.insert( CString( L"vid_05c6&pid_901c" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9100" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9101" ) ); 
      mDeviceIDs.insert( CString( L"vid_05c6&pid_9402" ) );
      mDeviceIDs.insert( CString( L"vid_05c6&pid_f005" ) );
      
      mpDriverOEM = DEVICE_OEM_QUALCOMM;
      mDriverTLA = L"";
   }
 
}

/*===========================================================================
METHOD:
   EnumerateDevices (Internal Method)

DESCRIPTION:
   Build a map of devices of the given device class
  
PARAMETERS:
   guid        [ I ] - Device class
   bPresent    [ I ] - Only enumerate devices that are attached?

RETURN VALUE:
   std::map <DEVINST, CString>
===========================================================================*/
std::map <DEVINST, CString> cDeviceManager::EnumerateDevices( 
   const GUID &               guid,
   bool                       bPresent )
{
   std::map <DEVINST, CString> retMap;

   DWORD flags = DIGCF_DEVICEINTERFACE;
   if (bPresent == true)
   {
      flags |= DIGCF_PRESENT;
   }

   HDEVINFO hSet;
   hSet = ::SetupDiGetClassDevs( &guid, 0, 0, flags );
   if (hSet != INVALID_HANDLE_VALUE)
   {
      DWORD idx = 0;

      // Buffer for device ID string
      const UINT maxSz = 1024;
      WCHAR buf[maxSz];

      // Device info (for DEVINST handle)
      SP_DEVINFO_DATA did;

      BOOL bRet = TRUE;
      while (bRet != FALSE)
      { 
         ::ZeroMemory( (LPVOID)&buf[0], (SIZE_T)maxSz );
         ::ZeroMemory( (LPVOID)&did, (SIZE_T)sizeof( did ) );
         did.cbSize = sizeof( did );

         // Get DEVINST handle
         bRet = ::SetupDiEnumDeviceInfo( hSet, idx++, &did );
         if (bRet == FALSE)
         {
            continue;
         }

         // Use that to get device ID string
         CONFIGRET rc = ::CM_Get_Device_ID( did.DevInst, &buf[0], maxSz, 0 );
         if (rc == CR_SUCCESS && buf[0] != 0) 
         {
            CString devID( (LPCWSTR)&buf[0] );
            bool bFound = false;
            
            CString tmpID = devID;
            tmpID.MakeLower();

            std::set <CString>::const_iterator pIter = mDeviceIDs.begin();
            while (pIter != mDeviceIDs.end())
            {
               CString tmpDID = *pIter++;
               if (tmpID.Find( tmpDID ) >= 0)
               {
                  bFound = true;
                  break;
               }
            }

            if (bFound == true)
            {
               retMap[did.DevInst] = devID;
            }
         }
      }

      ::SetupDiDestroyDeviceInfoList( hSet );
   }

   return retMap;
}

/*===========================================================================
METHOD:
   EnumerateDriverlessDecendants (Internal Method)

DESCRIPTION:
   Enumerate decendants of the given device with no driver loaded
   (as determined by comparing them to a set of devices present 
   with a driver)
  
PARAMETERS:
   hParent     [ I ] - Device ID handle of parent
   devPresent  [ I ] - Device ID handles of present devices
   devMap      [ O ] - Map to store descendants

RETURN VALUE:
   None
===========================================================================*/ 
void cDeviceManager::EnumerateDriverlessDecendants(
   DEVINST                             hParent,
   const std::map <DEVINST, CString> & devPresent,
   std::map <DEVINST, DEVINST> &       devMap )
{
   const UINT maxSz = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + 4096;

   WCHAR buf[maxSz];
   ::ZeroMemory( (LPVOID)&buf[0], (SIZE_T)maxSz );

   CString txt( L"" );

   DEVINST hChild = 0;
   DEVINST hSibling = 0;
   CONFIGRET crc = CM_Get_Child( &hChild, hParent, 0 );
   while (crc == CR_SUCCESS)
   {
      if (devPresent.find( hChild ) != devPresent.end())
      {
         // This child is a Gobi 2000 device with a driver loaded
         break;
      }

      buf[0] = 0;
      crc = CM_Get_Device_ID( hChild, (LPWSTR)&buf[0], maxSz, 0 );
      if (crc == CR_SUCCESS && buf[0] != 0)
      {
         CString devID( (LPCWSTR)&buf[0] );
         bool bFound = false;

         CString tmpID = devID;
         tmpID.MakeLower();

         std::set <CString>::const_iterator pIter = mDeviceIDs.begin();
         while (pIter != mDeviceIDs.end())
         {
            CString tmpDID = *pIter++;
            if (tmpID.Find( tmpDID ) >= 0)
            {
               bFound++;
               break;
            }
         }

         if (bFound == true)
         {
            devMap[hChild] = hParent;
         }
      }

      // Recurse
      EnumerateDriverlessDecendants( hChild, devPresent, devMap );

      hSibling = hChild;
      crc = CM_Get_Sibling( &hChild, hSibling, 0 );
   }
}

/*===========================================================================
METHOD:
   EnumerateDriverlessDevices (Internal Method)

DESCRIPTION:
   Build a map of devices that have no driver loaded
  
RETURN VALUE:
   std::map <DEVINST, DEVINST> - Map of device child ID to device parent ID
===========================================================================*/
std::map <DEVINST, DEVINST> cDeviceManager::EnumerateDriverlessDevices()
{
   std::map <DEVINST, DEVINST> retMap;

   // First enumerate present USB devices
   GUID guid = MY_GUID_USB_DEV;
   std::map <DEVINST, CString> devPresent = EnumerateDevices( guid, false );

   // Iterate through the all USB hubs
   guid = MY_GUID_USB_HUB;
   DWORD flags = DIGCF_DEVICEINTERFACE;
   HDEVINFO hSet = ::SetupDiGetClassDevs( &guid, 0, 0, flags );
   if (hSet != INVALID_HANDLE_VALUE)
   {
      DWORD idx = 0;

      // Device info (for DEVINST handle)
      SP_DEVINFO_DATA did;

      BOOL bRet = TRUE;
      while (bRet != FALSE)
      { 
         ::ZeroMemory( (LPVOID)&did, (SIZE_T)sizeof( did ) );
         did.cbSize = sizeof( did );

         // Get DEVINST handle
         bRet = ::SetupDiEnumDeviceInfo( hSet, idx++, &did );
         if (bRet == FALSE)
         {
            continue;
         }

         EnumerateDriverlessDecendants( did.DevInst, devPresent, retMap );
      }

      ::SetupDiDestroyDeviceInfoList( hSet );
   }

   return retMap;
}

/*===========================================================================
METHOD:
   RescanDevice (Internal Method)

DESCRIPTION:
   Rescan the given device
  
PARAMETERS:
   pDevID      [ I ] - Device ID string

RETURN VALUE:
   bool
===========================================================================*/
bool cDeviceManager::RescanDevice( LPCWSTR pDevID )
{
   // Sanity check argument
   if (pDevID == 0 || pDevID[0] == 0)
   {
      return false;
   }

   CString txt = L"";
   txt.Format( L"Rescanning %s", pDevID );
   mLog.Log( (LPCWSTR)txt );

   // Create a device info set to hold the device to remove
   HDEVINFO hSet = ::SetupDiCreateDeviceInfoList( 0, 0 );
   if (hSet == INVALID_HANDLE_VALUE)
   {
      DWORD ec = ::GetLastError();
      txt.Format( L"SetupDiCreateDeviceInfoList() failure [0x%08X]", ec );
      mLog.Log( (LPCWSTR)txt );

      return false;
   }

   SP_DEVINFO_DATA did;
   ::ZeroMemory( (LPVOID)&did, (SIZE_T)sizeof( did ) );
   did.cbSize = sizeof( did );

   // Add the device to the above set
   BOOL bRet = ::SetupDiOpenDeviceInfo( hSet, pDevID, 0, 0, &did );
   if (bRet == FALSE)
   {
      DWORD ec = ::GetLastError();
      txt.Format( L"SetupDiOpenDeviceInfo() failure [0x%08X]", ec );
      mLog.Log( (LPCWSTR)txt );

      ::SetupDiDestroyDeviceInfoList( hSet );
      return false;
   }

   // Setup the 'remove device' parameters
   SP_PROPCHANGE_PARAMS pcp;
   pcp.ClassInstallHeader.cbSize = sizeof( SP_CLASSINSTALL_HEADER );
   pcp.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
   pcp.StateChange = DICS_ENABLE;
   pcp.Scope = DICS_FLAG_GLOBAL;
   pcp.HwProfile = 0;

   PSP_CLASSINSTALL_HEADER pHdr = (PSP_CLASSINSTALL_HEADER)&pcp;
   bRet = ::SetupDiSetClassInstallParams( hSet, &did, pHdr, sizeof( pcp ) );
   if (bRet == FALSE)
   {
      DWORD ec = ::GetLastError();
      txt.Format( L"SetupDiSetClassInstallParams() failure [0x%08X]", ec );
      mLog.Log( (LPCWSTR)txt );

      ::SetupDiDestroyDeviceInfoList( hSet );
      return false;
   }

   bRet = ::SetupDiCallClassInstaller( DIF_PROPERTYCHANGE, hSet, &did );
   if (bRet == FALSE)
   {
      DWORD ec = ::GetLastError();
      txt.Format( L"SetupDiCallClassInstaller() failure [0x%08X]", ec );
      mLog.Log( (LPCWSTR)txt );

      ::SetupDiDestroyDeviceInfoList( hSet );
      return false;
   }

   ::SetupDiDestroyDeviceInfoList( hSet );
   return true;
}

/*===========================================================================
METHOD:
   RemoveDevice (Internal Method)

DESCRIPTION:
   Remove the given device
  
PARAMETERS:
   pDevID      [ I ] - Device ID string

RETURN VALUE:
   bool
===========================================================================*/
bool cDeviceManager::RemoveDevice( LPCWSTR pDevID )
{
   // Sanity check argument
   if (pDevID == 0 || pDevID[0] == 0)
   {
      return false;
   }

   CString txt = L"";
   txt.Format( L"Removing %s", pDevID );
   mLog.Log( (LPCWSTR)txt );

   // Create a device info set to hold the device to remove
   HDEVINFO hSet = ::SetupDiCreateDeviceInfoList( 0, 0 );
   if (hSet == INVALID_HANDLE_VALUE)
   {
      DWORD ec = ::GetLastError();
      txt.Format( L"SetupDiCreateDeviceInfoList() failure [0x%08X]", ec );
      mLog.Log( (LPCWSTR)txt );

      return false;
   }

   SP_DEVINFO_DATA did;
   ::ZeroMemory( (LPVOID)&did, (SIZE_T)sizeof( did ) );
   did.cbSize = sizeof( did );

   // Add the device to the above set
   BOOL bRet = ::SetupDiOpenDeviceInfo( hSet, pDevID, 0, 0, &did );
   if (bRet == FALSE)
   {
      DWORD ec = ::GetLastError();
      txt.Format( L"SetupDiOpenDeviceInfo() failure [0x%08X]", ec );
      mLog.Log( (LPCWSTR)txt );

      ::SetupDiDestroyDeviceInfoList( hSet );
      return false;
   }

   // Setup the 'remove device' parameters
   SP_REMOVEDEVICE_PARAMS rdp;
   rdp.ClassInstallHeader.cbSize = sizeof( SP_CLASSINSTALL_HEADER );
   rdp.ClassInstallHeader.InstallFunction = DIF_REMOVE;
   rdp.Scope = DI_REMOVEDEVICE_GLOBAL;
   rdp.HwProfile = 0;

   PSP_CLASSINSTALL_HEADER pHdr = (PSP_CLASSINSTALL_HEADER)&rdp;
   bRet = ::SetupDiSetClassInstallParams( hSet, &did, pHdr, sizeof( rdp ) );
   if (bRet == FALSE)
   {
      DWORD ec = ::GetLastError();
      txt.Format( L"SetupDiSetClassInstallParams() failure [0x%08X]", ec );
      mLog.Log( (LPCWSTR)txt );

      ::SetupDiDestroyDeviceInfoList( hSet );
      return false;
   }

   bRet = ::SetupDiCallClassInstaller( DIF_REMOVE, hSet, &did );
   if (bRet == FALSE)
   {
      DWORD ec = ::GetLastError();
      txt.Format( L"SetupDiCallClassInstaller() failure [0x%08X]", ec );
      mLog.Log( (LPCWSTR)txt );

      ::SetupDiDestroyDeviceInfoList( hSet );
      return false;
   }

   ::SetupDiDestroyDeviceInfoList( hSet );
   return true;
}

/*===========================================================================
METHOD:
   RemoveDriverService

DESCRIPTION:
   Remove the service associated with the given driver 
 
PARAMETERS:
   pService    [ I ] - Name of service to remove

RETURN VALUE:
   bool
===========================================================================*/
bool cDeviceManager::RemoveDriverService( LPCWSTR pService )
{
   // Sanity check argument
   if (pService == 0 || pService[0] == 0)
   {
      return false;
   }

   CString txt = L"";
   txt.Format( L"Removing %s", pService );
   mLog.Log( (LPCWSTR)txt );

   // Open the SCM (local machine, default DB, all access pass)
   SC_HANDLE hSCM = ::OpenSCManager( 0, 0, SC_MANAGER_ALL_ACCESS );
   if (hSCM == 0) 
   {
      DWORD ec = ::GetLastError();
      txt.Format( L"OpenSCManager() failure [0x%08X]", ec );
      mLog.Log( (LPCWSTR)txt );

      return false;
   }

   SC_HANDLE hService = ::OpenService( hSCM, pService, SERVICE_ALL_ACCESS );
   if (hService == 0) 
   {
      // This service was never installed
      DWORD ec = ::GetLastError();
      txt.Format( L"OpenService() failure [0x%08X]", ec );
      mLog.Log( (LPCWSTR)txt );

      ::CloseServiceHandle( hSCM );
      return false;
   }

   // Did we succeed in stopping the service?
   bool bStopped = false;

   // Stop the service
   SERVICE_STATUS ss;
   BOOL bOK = ::ControlService( hService, SERVICE_CONTROL_STOP, &ss );
   if (bOK != FALSE)
   {
      UINT count = 0;
      while (count++ < 10) 
      {
         ::Sleep( 1000 );

         bOK = ::QueryServiceStatus( hService, &ss );
         if (bOK == FALSE)
         {
            break;
         }

         if (ss.dwCurrentState != SERVICE_STOP_PENDING) 
         {
            break;
         }
      }

      if (ss.dwCurrentState == SERVICE_STOPPED)
      {
         bStopped = true;
      }
   }
   else
   {
      DWORD ec = ::GetLastError();
      txt.Format( L"ControlService( STOP ) failure [0x%08X]", ec );
      mLog.Log( (LPCWSTR)txt );
   }

   // Remove the service
   bOK = ::DeleteService( hService );
   if (bOK == FALSE)
   {
      DWORD ec = ::GetLastError();
      txt.Format( L"DeleteService() failure [0x%08X]", ec );
      mLog.Log( (LPCWSTR)txt );
   }

   ::CloseServiceHandle( hSCM );
   ::CloseServiceHandle( hService );

   return (bOK != FALSE);
}

/*===========================================================================
METHOD:
   SetupDriverEnvironment (Free Method)

DESCRIPTION:
   Setup the environment necessary for installing/unistalling drivers
 
PARAMETERS:
   path        [ I ] - Base path for the drivers files
   b64         [ I ] - 64-bit installation?

RETURN VALUE:
   bool
===========================================================================*/
bool cDeviceManager::SetupDriverEnvironment( 
   const CString &            path,
   bool                       b64,
   bool                       biswin7,
   bool                       bisLegacyEthernetDriver)
{
   // Assume failure
   bool bRC = false;
   /*if (mDeviceIDs.size() <= 0)
   {
#if DBG_ERROR_MESSAGE_BOX   
          CString txt;
          txt.Format( L"SetupDriverEnvironment:mDeviceIDs.size() <= 0");
          MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO);
#endif   
      return bRC;
   }*/

   CString pre7Path = path;
   CString win7Path = path;
   CString difxPath;

   pre7Path.Trim();
   win7Path.Trim();

   INT len = pre7Path.GetLength();
   if (len > 0)
   {
      if (pre7Path[len - 1] != _T('\\'))
      {
         pre7Path += _T('\\');
         win7Path += _T('\\');
      }
   }

   difxPath = pre7Path;

   if (b64 == false)
   {
      difxPath += L"..\\..\\DifxApi\\i386\\";
   }
   else
   {
      difxPath += L"..\\..\\DifxApi\\amd64\\";
   }

   // Load the DLL module?
   if (mhDLL == 0)
   {
      mhDLL = ::LoadLibrary( (LPCWSTR)(difxPath + L"difxapi.dll") );
      if (mhDLL == 0)
      {  
         CString txt;
         txt.Format( L"Error:Load difxapi() = %x, pre7path = %s", GetLastError(),pre7Path );
         mLog.Log((LPCWSTR)txt );
         mLog.Log( L"DifxAPI DLL load failed" );
#if DBG_ERROR_MESSAGE_BOX   
                   MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO);
#endif   
         return bRC;
      }
   }

   // Load the required Difx API functions
   LPCSTR pName = "DriverPackagePreinstallW";
   mpFnPreinstall = (pDifxPreInstall)::GetProcAddress( mhDLL, pName );

   pName = "DriverPackageInstallW";
   mpFnInstall = (pDifxInstall)::GetProcAddress( mhDLL, pName );

   pName = "DriverPackageUninstallW";
   mpFnUninstall = (pDifxInstall)::GetProcAddress( mhDLL, pName );

   if (mpFnPreinstall == 0 || mpFnInstall == 0 || mpFnUninstall == 0)
   {
      mLog.Log( L"DifxAPI DLL function address failure" );
#if DBG_ERROR_MESSAGE_BOX   
                CString txt;
                txt.Format( L"DifxAPI DLL function address failure");
                MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO);
#endif   
      return bRC;
   }

   // Set the driver INF paths
   CString serPath = pre7Path;
   serPath += L"qcser";
   serPath += mDriverTLA;
   serPath += L".inf";

   CString mdmPath = pre7Path;
   mdmPath += L"qcmdm";
   mdmPath += mDriverTLA;
   mdmPath += L".inf";

   CString filterPath = pre7Path;
   filterPath += L"qcfilter";
   filterPath += mDriverTLA;
   filterPath += L".inf";

   CString netPathPre7 = pre7Path;
   netPathPre7 += L"qcnet";
   netPathPre7 += mDriverTLA;
   netPathPre7 += L".inf";

   CString netPathWin7 = win7Path;
   netPathWin7 += L"qcwwan";
   netPathWin7 += mDriverTLA;
   netPathWin7 += L".inf";

   CString coinPath = pre7Path;
   coinPath += L"setup";
   coinPath += L".inf";

   CString coinDllPath = pre7Path;
   coinDllPath += L"QualcommCoInstaller";
   coinDllPath += L".dll";

   CString qdssPath = pre7Path;
   qdssPath += L"qdbusb";
   qdssPath += mDriverTLA;
   qdssPath += L".inf";
   
#if AUTO_CLOSE_UNSIGNED_WARN  
   CString SetupApiDllPath = pre7Path;
   SetupApiDllPath += L"SETUPAPI";
   SetupApiDllPath += L".DLL";
#endif
    
   DWORD rc = ::GetFileAttributes( serPath );
   if (rc == INVALID_FILE_ATTRIBUTES)
   {
      mLog.Log( L"Unable to locate serial INF file" );
#if DBG_ERROR_MESSAGE_BOX   
                      CString txt;
                      txt.Format( L"Unable to locate serial INF file");
                      MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO);
#endif   
      return bRC;
   }

   rc = ::GetFileAttributes( mdmPath );
   if (rc == INVALID_FILE_ATTRIBUTES)
   {
#if DBG_ERROR_MESSAGE_BOX   
                             CString txt;
                             txt.Format( L"Unable to locate modem INF file");
                             MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO);
#endif   
      mLog.Log( L"Unable to locate modem INF file" );
      return bRC;
   }

   rc = ::GetFileAttributes( filterPath );
   if (rc == INVALID_FILE_ATTRIBUTES)
   {
      mLog.Log( L"Unable to locate filter INF file" );
      /*return bRC;*/
   }
   /*0=false indicate use legacy driver is false, we should we should use WIN7 usb drivers*/
   if((true == biswin7) && (false == bisLegacyEthernetDriver))
   {
       rc = ::GetFileAttributes( netPathWin7 );
       if (rc == INVALID_FILE_ATTRIBUTES)
       {
          mLog.Log( L"Unable to locate network INF file" );
#if DBG_ERROR_MESSAGE_BOX   
                                       CString txt;
                                       txt.Format( L"Unable to locate network INF file win7");
                                       MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO);
#endif   
          return bRC;
       }
       mQCNetPath = netPathWin7;
       mQCAlsoDeletePath = netPathPre7;
   }
   else
   {
       rc = ::GetFileAttributes( netPathPre7 );
       if (rc == INVALID_FILE_ATTRIBUTES)
       {
          mLog.Log( L"Unable to locate network INF file" );
#if DBG_ERROR_MESSAGE_BOX   
                                                 CString txt;
                                                 txt.Format( L"Unable to locate network INF file pre7");
                                                 MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO);
#endif   
          return bRC;
       }
       mQCNetPath = netPathPre7;
       mQCAlsoDeletePath = netPathWin7;
   }
   
#if AUTO_CLOSE_UNSIGNED_WARN	  
   mQCSetupApiDllPath = SetupApiDllPath;
#endif

   mQCSerialPath = serPath;
   mQCModemPath = mdmPath;
   mQCFilterPath = filterPath;
   mQCQdssPath = qdssPath;
   
#if CALL_DRIVER_COINSTALLER   
   mQCCoinstallerPath = coinPath;
   mQCCoinstallerDllPath = coinDllPath;
#endif   
   bRC = true;
   return bRC;
}

/*===========================================================================
METHOD:
   DeleteRegValue (Internal Method)

DESCRIPTION:
   Delete a registry key
  
PARAMETERS:
   pKey        [ I ] - Registry key (under HKLM)
   pName       [ I ] - Registry value name (under key)

RETURN VALUE:
   bool
===========================================================================*/
bool cDeviceManager::DeleteRegValue(
   LPCWSTR                    pKey,
   LPCWSTR                    pName )
{
   // Assume failure
   bool bRC = false;

   CString txt = L"";
   if (pKey == 0 || pKey[0] == 0 || pName == 0 || pName[0] == 0)
   {
      txt = L"Invalid argument passed to DeleteRegValue()";
      mLog.Log( (LPCWSTR)txt );
   }

   HKEY hKey = 0;
   LONG rc = ::RegOpenKeyEx ( HKEY_LOCAL_MACHINE, 
                              pKey, 
                              0,
                              KEY_ALL_ACCESS | DELETE,
                              &hKey );

   if (rc != ERROR_SUCCESS)
   {
      txt.Format( L"RegOpenKeyEx( %s ) = %u", pKey, rc );
      mLog.Log( (LPCWSTR)txt );

      return bRC;
   }

   rc = ::RegDeleteValue( hKey, pName );
   if (rc != ERROR_SUCCESS)
   {
      txt.Format( L"RegDeleteKey( %s ) = %u", pName, rc );
      mLog.Log( (LPCWSTR)txt );
   }
   else
   {
      bRC = true;
   }

   // Close registry key
   ::RegCloseKey( hKey );
   return bRC;
}

/*===========================================================================
METHOD:
   WriteRegValue (Internal Method)

DESCRIPTION:
   Write a registry value
  
PARAMETERS:
   pKey        [ I ] - Registry key (under HKLM)
   pName       [ I ] - Registry value name (under key)
   val         [ I ] - DWORD value to write

RETURN VALUE:
   bool
===========================================================================*/
bool cDeviceManager::WriteRegValue(
   LPCWSTR                    pKey,
   LPCWSTR                    pName,
   DWORD                      val )
{
   // Assume failure
   bool bRC = false;

   CString txt = L"";
   if (pKey == 0 || pKey[0] == 0 || pName == 0 || pName[0] == 0)
   {
      txt = L"Invalid argument passed to WriteRegValue()";
      mLog.Log( (LPCWSTR)txt );
   }

   HKEY hKey = 0;
   LONG rc = ::RegCreateKeyEx( HKEY_LOCAL_MACHINE, 
                               pKey, 
                               0,
                               0,
                               REG_OPTION_NON_VOLATILE,
                               KEY_WRITE,
                               0,
                               &hKey,
                               0 );

   if (rc != ERROR_SUCCESS)
   {
      txt.Format( L"RegCreateKeyEx( %s ) = %u", pKey, rc );
      mLog.Log( (LPCWSTR)txt );

      return bRC;
   }

   DWORD vt = REG_DWORD;
   rc = ::RegSetValueEx( hKey, pName, 0, vt, (PBYTE)&val, sizeof(DWORD) );
   if (rc != ERROR_SUCCESS)
   {
      txt.Format( L"RegSetValueEx( %s ) = %u", pName, rc );
      mLog.Log( (LPCWSTR)txt );
   }
   else
   {
      bRC = true;
   }

   // Close registry key
   ::RegCloseKey( hKey );
   return bRC;
}

/*===========================================================================
METHOD:
   IsDevicePresent (Public Method)

DESCRIPTION:
   Is there a device present in the system?

RETURN VALUE:
   bool
===========================================================================*/
bool cDeviceManager::IsDevicePresent()
{
   std::map <DEVINST, DEVINST> retMap;

   // By using an empty map we will be returned all Gobi descendants 
   // of USB hubs regardless of whether or not a driver is loaded for
   // each discovered descendant
   std::map <DEVINST, CString> devEmpty;

   // Iterate through the all USB hubs
   GUID guid = MY_GUID_USB_HUB;
   DWORD flags = DIGCF_DEVICEINTERFACE;
   HDEVINFO hSet = ::SetupDiGetClassDevs( &guid, 0, 0, flags );
   if (hSet != INVALID_HANDLE_VALUE)
   {
      DWORD idx = 0;

      // Device info (for DEVINST handle)
      SP_DEVINFO_DATA did;

      BOOL bRet = TRUE;
      while (bRet != FALSE)
      { 
         ::ZeroMemory( (LPVOID)&did, (SIZE_T)sizeof( did ) );
         did.cbSize = sizeof( did );

         // Get DEVINST handle
         bRet = ::SetupDiEnumDeviceInfo( hSet, idx++, &did );
         if (bRet == FALSE)
         {
            continue;
         }

         EnumerateDriverlessDecendants( did.DevInst, devEmpty, retMap );
		 if((ULONG)retMap.size() > 0)
		 {
             /*It's normal for device is absent in computer,if device is connected,
                              we reenumerate bus to install drivers.*/
             CONFIGRET crc = ::CM_Reenumerate_DevNode(did.DevInst,CM_REENUMERATE_ASYNCHRONOUS);
			 if(crc == CR_SUCCESS)
			 {
			 }
		 }
      }
      ::SetupDiDestroyDeviceInfoList( hSet );
   }

   // Device is present if we found a Gobi descendant of a USB hub
   return (ULONG)retMap.size() > 0;
}

/*===========================================================================
METHOD:
   RescanDevices (Public Method)

DESCRIPTION:
   Rescan device tree for inactive devices, when one is found request 
   the system load a driver for the device

RETURN VALUE:
   None
===========================================================================*/
void cDeviceManager::RescanDevices()
{
   const UINT maxSz = 1024;

   std::map <DEVINST, DEVINST> orphans = EnumerateDriverlessDevices();
   std::map <DEVINST, DEVINST>::const_iterator pIter = orphans.begin();
   while (pIter != orphans.end())
   {
      DEVINST hDev = pIter->first;
      DEVINST hParent = pIter->second;
      pIter++;

      CString txt = L"";
      txt.Format( L"Rescanning %u", (ULONG)hDev );
      mLog.Log( (LPCWSTR)txt );

      // Try the quick approach for the device
      CONFIGRET crc = ::CM_Setup_DevNode( hDev, CM_SETUP_DEVNODE_READY );
      if (crc == CR_SUCCESS)
      {
         // On to the next one
         continue;
      }

      // Failed for the device, try the parent (using an alternative approach)
      WCHAR buf[maxSz];
      ::ZeroMemory( (LPVOID)&buf[0], (SIZE_T)maxSz );

      crc = CM_Get_Device_ID( hParent, (LPWSTR)&buf[0], maxSz, 0 );
      if (crc == CR_SUCCESS && buf[0] != 0)
      {
         RescanDevice( (LPCWSTR)&buf[0] );
      }
   }
}

/*===========================================================================
METHOD:
   RemoveDevices (Public Method)

DESCRIPTION:
   Remove all instances of the devices we support
  
RETURN VALUE:
   None
===========================================================================*/
void cDeviceManager::RemoveDevices()
{
   std::map <DEVINST, CString> netDevs = EnumerateDevices( MY_GUID_NET );
   std::map <DEVINST, CString> modemDevs = EnumerateDevices( MY_GUID_MODEM );
   std::map <DEVINST, CString> portDevs = EnumerateDevices( MY_GUID_PORT );
   std::map <DEVINST, CString> usbDevs = EnumerateDevices( MY_GUID_USB_DEV );
   
   std::map <DEVINST, CString>::const_iterator pIter = netDevs.begin();
   while (pIter != netDevs.end())
   {
      LPCWSTR pDevID = (LPCWSTR)pIter->second;
      RemoveDevice( pDevID );

      pIter++;
   }

   pIter = modemDevs.begin();
   while (pIter != modemDevs.end())
   {
      LPCWSTR pDevID = (LPCWSTR)pIter->second;
      RemoveDevice( pDevID );

      pIter++;
   }

   pIter = portDevs.begin();
   while (pIter != portDevs.end())
   {
      LPCWSTR pDevID = (LPCWSTR)pIter->second;
      RemoveDevice( pDevID );

      pIter++;
   }

   pIter = usbDevs.begin();
   while (pIter != usbDevs.end())
   {
      LPCWSTR pDevID = (LPCWSTR)pIter->second;
      RemoveDevice( pDevID );

      pIter++;
   }
}
#define QC_INF_NAME             _T("qcusbser.inf")
#define QC_CLASS_GUID           _T("{4D36E978-E325-11CE-BFC1-08002BE10318}")
#define QC_CLASS                _T("Ports")
#define QC_PROVIDER             _T("Qualcomm Incorporated")

BOOL ScanForHardwareChanges() 
{
    DEVINST     devInst;
    CONFIGRET   status;
    
    status = CM_Locate_DevNode(&devInst, NULL, CM_LOCATE_DEVNODE_NORMAL);
    if (status != CR_SUCCESS) 
    {
        return FALSE;
    }
    
    status = CM_Reenumerate_DevNode(devInst, 0);
    if (status != CR_SUCCESS) 
    {
        return FALSE;
    }
    return TRUE;
}    

LPTSTR * QCGetMultiSzIndexArray(LPTSTR QCMultiSz)
{
    LPTSTR qcscan;
    LPTSTR * qcarray;
    int qcelements;

    for(qcscan = QCMultiSz, qcelements = 0; qcscan[0] ;qcelements++) 
    {
        qcscan += lstrlen(qcscan)+1;
    }
    qcarray = new LPTSTR[qcelements+2];
    
    if(qcarray == NULL) 
    {
        return NULL;
    }

    qcarray[0] = QCMultiSz;
    qcarray++;

    if(qcelements) 
    {
        for(qcscan = QCMultiSz, qcelements = 0; qcscan[0]; qcelements++) 
	    {
            qcarray[qcelements] = qcscan;
            qcscan += lstrlen(qcscan)+1;
        }
    }
    qcarray[qcelements] = NULL;
    return qcarray;
}

#if UNINSTALL_OLD
///////////////////////////////////////////////////////////////
static BOOL CallUninstallOnce = FALSE;
bool cDeviceManager::UninstallPrevDrivers(const CString & infFilePath,const char *pDevId,const char *pCopyFileSection)
{
   BOOL rebootRequired = FALSE;
   // DevProperties devProperties;
   // InfFile infFile(infFilePath,infFilePath);
   // devProperties = DevProperties();
   //if (!devProperties.DevId(pDevId))
     /*goto err*/;
   {
     // Stack stack;
 
	 /* MURALI COMMNETED */
     // if (!DisableDevices(infFile, &devProperties, &rebootRequired, &stack)) {
     //  goto err;
     // }
     // if (!RemoveDevicesUtils(infFile, &devProperties, &rebootRequired))
     //  goto err;
	 /* MURALI END COMMNETED */
   }
 
   /* MURALI Added */
   if (CallUninstallOnce == FALSE)
   {
	  CallUninstallOnce = TRUE;
      // if (!InfFile::UninstallInfFiles(infFilePath))
      //  goto err;
   }
   /* MURALI End Added */
   // if (!infFile.DeleteExecFile(pCopyFileSection,0))
   //  goto err;
   /* MURALI COMMNETED */
   // if (!InfFile::UninstallAllInfFiles(infFile.ClassGUID(FALSE), infFile.Class(FALSE), infFile.Provider(FALSE)))
   //  goto err;
   /* MURALI END COMMNETED */
   if(TRUE == rebootRequired)
   {
       // Trace(_T("You must restart computer to cleanup all old version drivers\n"));
       restartRequired=1;
   }
   return 0;
 
   // err:
 
   // Trace(_T("\nUninstall not completed!\n"));
   return  1;

}
#endif


LPTSTR * QCGetDevMultiSz(HDEVINFO QCDevs, PSP_DEVINFO_DATA QCDevInfo, DWORD QCProp)
{
    LPTSTR qcbuffer;
    DWORD qcsize;
    DWORD qcreqSize;
    DWORD qcdataType;
    LPTSTR * qcarray;
    DWORD qcszChars;

    qcsize = 8192;
    qcbuffer = new TCHAR[(qcsize/sizeof(TCHAR))+2];
    
    if(qcbuffer == NULL) 
    {
        return NULL;
    }
    while(!SetupDiGetDeviceRegistryProperty(QCDevs,QCDevInfo,QCProp,&qcdataType,(LPBYTE)qcbuffer,qcsize,&qcreqSize)) 
    {
        if(GetLastError() != ERROR_INSUFFICIENT_BUFFER) 
	    {
            goto failed;
        }
        if(qcdataType != REG_MULTI_SZ) 
	    {
            goto failed;
        }
        qcsize = qcreqSize;
        delete [] qcbuffer;
        qcbuffer = new TCHAR[(qcsize/sizeof(TCHAR))+2];
        if(qcbuffer == NULL) 
	    {
            goto failed;
        }
    }
    qcszChars = qcreqSize/sizeof(TCHAR);
    qcbuffer[qcszChars] = TEXT('\0');
    qcbuffer[qcszChars+1] = TEXT('\0');

    qcarray = QCGetMultiSzIndexArray(qcbuffer);

    if(qcarray) 
    {
        return qcarray;
    }

failed:
    if(qcbuffer) 
    {
        delete [] qcbuffer;
    }
    return NULL;
}

BOOL QCCompareFilterList ( LPCTSTR Item)
{
   std::set <CString>::const_iterator pIter = FilterList.begin();
   while (pIter != FilterList.end())
   {
      CString tmpStr = *pIter++;
      CString tmpID(Item);
      tmpID.MakeLower();
      if (tmpID.Find( tmpStr ) >= 0)
      {
		  return TRUE;
	  }
   }
   return FALSE;
}

BOOL QCWildCardMatch(LPCTSTR Item, const QC_IdEnt & MatchEntry)
{
    LPCTSTR scanItem;
    LPCTSTR wildMark;
    LPCTSTR nextWild;
    size_t matchlen;

	if ( QCCompareFilterList( Item ) == TRUE)
    {
       return FALSE;
    }

    if(MatchEntry.QC_Wildchar == NULL) 
	{
        if (_tcsicmp(Item,MatchEntry.QC_Str)) 
			return FALSE;
		else 
			return TRUE;
    }
    if(_tcsnicmp(Item,MatchEntry.QC_Str,MatchEntry.QC_Wildchar-MatchEntry.QC_Str) != 0) 
	{
        return FALSE;
    }
    wildMark = MatchEntry.QC_Wildchar;
    scanItem = Item + (MatchEntry.QC_Wildchar-MatchEntry.QC_Str);

    for(;wildMark[0];) 
    {
        if(wildMark[0] == TEXT('*')) 
		{
            wildMark = CharNext(wildMark);
            continue;
        }
        nextWild = _tcschr(wildMark,TEXT('*'));
        if(nextWild) 
		{
            matchlen = nextWild-wildMark;
        } 
		else 
		{
            size_t scanlen = lstrlen(scanItem);
            matchlen = lstrlen(wildMark);
            if(scanlen < matchlen)
			{
                return FALSE;
            }
            if (_tcsicmp(scanItem+scanlen-matchlen,wildMark))
				return FALSE;
			else
				return TRUE;
        }
        if(_istalpha(wildMark[0])) 
		{
            TCHAR u = _totupper(wildMark[0]);
            TCHAR l = _totlower(wildMark[0]);
            while(scanItem[0] && scanItem[0]!=u && scanItem[0]!=l) 
			{
                scanItem = CharNext(scanItem);
            }
            if(!scanItem[0]) 
			{
                return FALSE;
            }
        } 
		else 
		{
            scanItem = _tcschr(scanItem,wildMark[0]);
            if(!scanItem) 
			{
                return FALSE;
            }
        }
        if(_tcsnicmp(scanItem,wildMark,matchlen)!=0) 
		{
            scanItem = CharNext(scanItem);
            continue;
        }
        scanItem += matchlen;
        wildMark += matchlen;
    }
    if (wildMark[0])
		return FALSE;
	else
		return TRUE;
}

BOOL QCWildCompareHwIds(PZPWSTR Ary, const QC_IdEnt & MthEnt)
{
    if(Ary != NULL) 
	{
        while(Ary[0] != NULL) 
		{
            if(QCWildCardMatch(Ary[0],MthEnt)) 
			{
                return TRUE;
            }
            Ary++;
        }
    }
    return FALSE;
}

QC_IdEnt QCGetIdType(LPCTSTR QCId)
{
    QC_IdEnt QCEntry;
    QCEntry.QC_InstId = FALSE;
    QCEntry.QC_Wildchar = NULL;
    QCEntry.QC_Str = QCId;
    QCEntry.QC_Wildchar = _tcschr(QCEntry.QC_Str,TEXT('*'));
    return QCEntry;
}

void QCDelMultiSz(PZPWSTR Ary)
{
    if(Ary) 
	{
        Ary--;
        if(Ary[0]) 
		{
            delete [] Ary[0];
        }
        delete [] Ary;
    }
}

int QCRemoveDevice(HDEVINFO QCDevs, PSP_DEVINFO_DATA QCDevInfo, DWORD Index, LPVOID Context)
{
    SP_REMOVEDEVICE_PARAMS rmdParams;
    SP_DEVINSTALL_PARAMS devParams;
    LPCTSTR action = NULL;
    TCHAR devID[MAX_DEVICE_ID_LEN];
    BOOL b = TRUE;
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if((!SetupDiGetDeviceInfoListDetail(QCDevs,&devInfoListDetail)) ||
         (CM_Get_Device_ID_Ex(QCDevInfo->DevInst,devID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS))
	{
        return QC_OK;
    }
    rmdParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
    rmdParams.ClassInstallHeader.InstallFunction = DIF_REMOVE;
    rmdParams.Scope = DI_REMOVEDEVICE_GLOBAL;
    rmdParams.HwProfile = 0;
    if(!SetupDiSetClassInstallParams(QCDevs,QCDevInfo,&rmdParams.ClassInstallHeader,sizeof(rmdParams)) ||
       !SetupDiCallClassInstaller(DIF_REMOVE,QCDevs,QCDevInfo))
	{
        action = L"FAIL";
    } 
	else 
	{
        devParams.cbSize = sizeof(devParams);
        if(SetupDiGetDeviceInstallParams(QCDevs,QCDevInfo,&devParams) && (devParams.Flags & (DI_NEEDRESTART|DI_NEEDREBOOT))) 
		{
            action = L"REBOOT";
        } 
		else 
		{
            action = L"SUCCESS";
        }
    }
    CM_Reenumerate_DevNode(QCDevInfo->DevInst, 0);
    _tprintf(TEXT("%-60s: %s\n"),devID,action);
    return QC_OK;
}

int QCEnumerateDevicesandRemove( LPCTSTR qchwId)
{
    HDEVINFO qcdevs = INVALID_HANDLE_VALUE;
    int failcode = QC_FAIL;
    SP_DEVINFO_LIST_DETAIL_DATA devInfoListDetail;
    int retcode;
    BOOL all = FALSE;
    BOOL doSearch = TRUE;
    int argIndex = 0;
    int skip = 0;
    GUID cls;
    QC_IdEnt * templ = NULL;
    SP_DEVINFO_DATA qcdevInfo;
    BOOL match;
    DWORD numClass = 0;
    DWORD devIndex;

    templ = new QC_IdEnt[1];
    if(!templ) 
    {
        goto qcfinal;
    }

    templ[argIndex] = QCGetIdType(qchwId);

    if (templ[argIndex].QC_Wildchar || !templ[argIndex].QC_InstId) 
    {
        doSearch = TRUE;
    }

    if(doSearch) 
    {
        qcdevs = SetupDiGetClassDevsEx(NULL,
                                     NULL,
                                     NULL,
                                     DIGCF_ALLCLASSES,
                                     NULL,
                                     NULL,
                                     NULL);
	

    } 
    else 
    {
        qcdevs = SetupDiCreateDeviceInfoListEx(numClass ? &cls : NULL,
                                             NULL,
                                             NULL,
                                             NULL);
    }
    if(qcdevs == INVALID_HANDLE_VALUE) 
    {
        goto qcfinal;
    }

    if(templ[argIndex].QC_InstId) 
    {
        SetupDiOpenDeviceInfo(qcdevs,templ[0].QC_Str,NULL,0,NULL);
    }

    devInfoListDetail.cbSize = sizeof(devInfoListDetail);
    if(!SetupDiGetDeviceInfoListDetail(qcdevs,&devInfoListDetail)) 
    {
        goto qcfinal;
    }

    qcdevInfo.cbSize = sizeof(qcdevInfo);
    for(devIndex=0;SetupDiEnumDeviceInfo(qcdevs,devIndex,&qcdevInfo);devIndex++) 
    {
        if(doSearch) 
	    {
            for(argIndex=0,match=FALSE;(argIndex<1) && !match;argIndex++) 
	        {
                LPTSTR *qccompatIds = NULL;
                LPTSTR *qchwIds = NULL;
                TCHAR qcdevID[MAX_DEVICE_ID_LEN];
                if(CM_Get_Device_ID_Ex(qcdevInfo.DevInst,qcdevID,MAX_DEVICE_ID_LEN,0,devInfoListDetail.RemoteMachineHandle)!=CR_SUCCESS) 
		        {
                    qcdevID[0] = TEXT('\0');
                }

                if(templ[argIndex].QC_InstId) 
		        {
                    if(QCWildCardMatch(qcdevID,templ[argIndex])) 
		            {
                        match = TRUE;
                    }
                } 
		        else 
		        {
                    qchwIds = QCGetDevMultiSz(qcdevs,&qcdevInfo,SPDRP_HARDWAREID);
                    qccompatIds = QCGetDevMultiSz(qcdevs,&qcdevInfo,SPDRP_COMPATIBLEIDS);

                    if(QCWildCompareHwIds(qchwIds,templ[argIndex]) ||
                        QCWildCompareHwIds(qccompatIds,templ[argIndex])) 
		            {
                        match = TRUE;
                    }
                }
                QCDelMultiSz(qchwIds);
                QCDelMultiSz(qccompatIds);
            }
        } 
	    else 
	    {
            match = TRUE;
        }
        if(match) 
	    {
            retcode = QCRemoveDevice(qcdevs,&qcdevInfo,devIndex,NULL);
            if(retcode) 
	        {
                failcode = retcode;
                goto qcfinal;
            }
        }
    }

    failcode = QC_OK;

qcfinal:
    if(templ) 
    {
        delete [] templ;
    }
    if(qcdevs != INVALID_HANDLE_VALUE) 
    {
        SetupDiDestroyDeviceInfoList(qcdevs);
    }

    gExecutionMode = EXEC_MODE_REMOVE_OEM;
    MainRemovalTask();

    return failcode;
}

/*===========================================================================
METHOD:
   InstallDrivers (Free Method)

DESCRIPTION:
   Install drivers
 
PARAMETERS:
   path        [ I ] - Base path for the drivers files
   b64         [ I ] - 64-bit installation?

RETURN VALUE:
   bool
===========================================================================*/
bool cDeviceManager::InstallDrivers( 
   const CString &            path,
   bool                       b64,
   bool                       biswin7,
   bool                       bisLegacyEthernetDriver)
{
   // Assume failure
   bool bRC = false;
#if 0
   if (mDeviceIDs.size() <= 0)
   {
      return bRC;
   }


   // Remove the Acer services from previous installations
   if (mDriverTLA == L"ad")
   {
      CString svcNameSer = L"qcusbseracr2k";
      CString svcNameNet = L"qcusbnetacr2k";
      CString svcNameFilter = L"qcfilteracr2k";

      RemoveDriverService( (LPCWSTR)svcNameSer );
      RemoveDriverService( (LPCWSTR)svcNameNet );
      RemoveDriverService( (LPCWSTR)svcNameFilter );
   }

#endif
#if DBG_ERROR_MESSAGE_BOX   
   MessageBox(NULL, path,NULL,MB_YESNO);
#endif
   bool bSetup = SetupDriverEnvironment( path, b64 , biswin7, bisLegacyEthernetDriver);
   if (bSetup == false)
   {
      return bRC;
   }

#if UNINSTALL_OLD
   // Silent(1);
   FilterList.insert( CString( L"vid_05c6&pid_9204" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9205" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9208" ) );
   FilterList.insert( CString( L"vid_05c6&pid_920b" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9214" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9215" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9224" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9225" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9234" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9235" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9244" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9245" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9254" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9255" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9264" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9265" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9284" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9285" ) );
   FilterList.insert( CString( L"vid_05c6&pid_3196" ) );
   FilterList.insert( CString( L"vid_05c6&pid_9018&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_901a" ) );
   FilterList.insert( CString( L"vid_05c6&pid_901d&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_901e&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_901f&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_901e&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_9020&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_9022&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_9023&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_9024&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_9025&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_9029&mi_00") );
   FilterList.insert( CString( L"vid_05c6&pid_902b") );
   FilterList.insert( CString( L"vid_05c6&pid_902d&mi_02") );
   FilterList.insert( CString( L"vid_05c6&pid_9030&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_9031&mi_02") );
   FilterList.insert( CString( L"vid_05c6&pid_9035&mi_02") );
   FilterList.insert( CString( L"vid_05c6&pid_9037&mi_02") );
   FilterList.insert( CString( L"vid_05c6&pid_9039&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_903a&mi_02") );
   FilterList.insert( CString( L"vid_05c6&pid_903b&mi_02") );
   FilterList.insert( CString( L"vid_05c6&pid_903d&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_903f&mi_03") );
   FilterList.insert( CString( L"vid_05c6&pid_9042&mi_03") );
   FilterList.insert( CString( L"vid_05c6&pid_9044&mi_02") );
   FilterList.insert( CString( L"vid_05c6&pid_9046&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_9049&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_904b&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_9053&mi_02") );
   FilterList.insert( CString( L"vid_05c6&pid_9056&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_9059&mi_02") );
   FilterList.insert( CString( L"vid_05c6&pid_905a&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_9060&mi_03") );
   FilterList.insert( CString( L"vid_05c6&pid_9064&mi_01") );
   FilterList.insert( CString( L"vid_05c6&pid_9065&mi_03") );

   // call remove
   QCEnumerateDevicesandRemove(L"usb\\vid_05c6*");
   
   // Sleep for 1.5 sec to make sure all devices are removed.
   Sleep(1500);

   UninstallPrevDrivers(mQCSerialPath,NULL,"\\system32\\drivers\\qcusbser.sys");

   //InfFile infFile(_T("qcmdm.inf"), NULL/**/);
   UninstallPrevDrivers(mQCModemPath,NULL,"\\system32\\QualcommCoInstaller.dll");
   
   //InfFile infFile(_T("qcnet.inf"), NULL/**/);/*"qcusbwwan.inf"*/
   UninstallPrevDrivers(mQCNetPath,NULL,"\\system32\\drivers\\qcusbnet.sys");

   UninstallPrevDrivers(mQCNetPath,NULL,"\\system32\\drivers\\qcusbwwan.sys");

   UninstallPrevDrivers(mQCQdssPath,NULL,"\\system32\\drivers\\qdbusb.sys");

   UninstallPrevDrivers(mQCFilterPath,NULL,"\\system32\\drivers\\qcusbfilter.sys");

#endif

if(false == b64)  
{
#if CALL_DRIVER_COINSTALLER  
   HINF SetupInf;               
   UINT ErrorLine;           
   
   SetupInf = SetupOpenInfFile (
      mQCCoinstallerPath,
      NULL,              
      INF_STYLE_WIN4,    
      &ErrorLine         
   );
   if (INVALID_HANDLE_VALUE == SetupInf)
   {
      CString txt;
      txt.Format(L"The function SetupOpenInfFile's error code is %d.", GetLastError());
      mLog.Log( (LPCWSTR)txt );
      return bRC;
   }
#if DBG_ERROR_MESSAGE_BOX   
   CString txt;
   txt.Format( L"Install coinstaller");
   MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO);
#endif   
   BOOL bResult = SetupInstallFromInfSection( 
            NULL, 
            SetupInf, 
            L"DefaultInstall", 
            SPINST_REGISTRY,//SPINST_REGISTRY | SPINST_INIFILES, 
            NULL, 
            NULL,      
            0,         
            NULL,      
            NULL,     
            NULL, 
            NULL);
   if (!bResult)
   {
      SetupCloseInfFile(SetupInf);
#if DBG_ERROR_MESSAGE_BOX   
      CString txt;
      txt.Format(L"The function SetupOpenInfFile's error code is %d.", GetLastError());
      mLog.Log( (LPCWSTR)txt );
#endif      
      return bRC;
   }
   
   SetupCloseInfFile(SetupInf);
   TCHAR sysBuffer[100];
   ExpandEnvironmentStrings(L"%systemroot%", sysBuffer, 100);
   CString sysPath = sysBuffer;
   sysPath += L"\\system32\\QualcommCoInstaller.dll";
   bResult = SetFileAttributes(sysPath, FILE_ATTRIBUTE_NORMAL);
   if (!bResult)
   {
      CString txt;
      txt.Format(L"Failed to set attribute to %s, error %d\n", sysPath, GetLastError());
      mLog.Log( (LPCWSTR)txt );
   }
   bResult = CopyFile(mQCCoinstallerDllPath, sysPath, FALSE);
   if (!bResult)
   {
      CString txt;
      txt.Format(L"Failed to copy %s, error %d\n", mQCCoinstallerDllPath, GetLastError());
      mLog.Log( (LPCWSTR)txt );
   }
#endif
}

   std::vector <CString> infFiles;
   infFiles.push_back( mQCFilterPath );
   infFiles.push_back( mQCSerialPath );
   infFiles.push_back( mQCModemPath );
   infFiles.push_back( mQCNetPath );
   infFiles.push_back( mQCQdssPath );
#if 0   
   infFiles.push_back( mQCFilterPath );
#endif   
   ULONG fileCount = (ULONG)infFiles.size();

   DWORD flag = DRIVER_PACKAGE_LEGACY_MODE;
   
#if AUTO_CLOSE_UNSIGNED_WARN  
   g_hLib = LoadLibrary(mQCSetupApiDllPath);
   if (NULL == g_hLib)
   {
       CString txt;
       txt.Format(L"Load library failed %s!",mQCSetupApiDllPath);
       mLog.Log((LPCWSTR)txt);
   }
   else
   {
       BOOL b = Allocate();
   }
#endif
   for (ULONG i = 0; i < fileCount; i++)
   {
      CString infFile = infFiles[i];

      DWORD rc = mpFnPreinstall( (LPCWSTR)infFile, flag );
      if (rc != ERROR_SUCCESS && rc != ERROR_ALREADY_EXISTS)
      {
         CString txt;
         txt.Format( L"Preinstall( %s ) = %u, flag = 0x%x", (LPCWSTR)infFile, rc,flag);
         mLog.Log( (LPCWSTR)txt );
#if 0
         MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif
         /*return bRC;*/
      }
	  CString txt;
	  txt.Format( L"Preinstall( %s ) = %u, flag = 0x%x", (LPCWSTR)infFile, rc,flag);
	  mLog.Log( (LPCWSTR)txt );

      BOOL bRebootFlag = FALSE;
#if DBG_ERROR_MESSAGE_BOX  
      txt.Format( L"Install Drivers %d, %s",i,infFile);
      MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif      
      rc = mpFnInstall( (LPCWSTR)infFile, flag, 0, &bRebootFlag );
      if ( (rc != ERROR_SUCCESS) 
      &&   (rc != ERROR_NO_MORE_ITEMS)
      &&   (rc != ERROR_NO_SUCH_DEVINST) )
      {
		 CString txt;
         txt.Format( L"Install( %s ) = %u, flag = 0x%x", (LPCWSTR)infFile, rc,flag);
         mLog.Log( (LPCWSTR)txt );
#if 0
         MessageBox(NULL, (LPCWSTR)txt,NULL,MB_YESNO); 
#endif
         /*return bRC;*/
      }
	  txt.Format( L"Install( %s ) = %u, flag = 0x%x", (LPCWSTR)infFile, rc,flag);
	  mLog.Log( (LPCWSTR)txt );
   }

   ScanForHardwareChanges();
   
#if AUTO_CLOSE_UNSIGNED_WARN	 
   if(NULL != g_hLib)
   {
       Release();
   }
#endif

   bRC = true;
   return bRC;
}

/*===========================================================================
METHOD:
   UninstallDrivers (Free Method)

DESCRIPTION:
   Uninstall drivers
 
PARAMETERS:
   path        [ I ] - Base path for the drivers files
   b64         [ I ] - 64-bit installation?

RETURN VALUE:
   bool
===========================================================================*/
bool cDeviceManager::UninstallDrivers( 
   const CString &            path,
   bool                       b64, 
   bool                       biswin7)
{
   // Assume failure
   bool bRC = false;
   /*if (mDeviceIDs.size() <= 0)
   {
      return bRC;
   }*/

   bool bSetup = SetupDriverEnvironment( path, b64, biswin7,false);
   if (bSetup == false)
   {
      return bRC;
   }

   std::vector <CString> infFiles;
   infFiles.push_back( mQCNetPath );
   infFiles.push_back( mQCModemPath );
   infFiles.push_back( mQCSerialPath );
   infFiles.push_back( mQCQdssPath );
   infFiles.push_back( mQCFilterPath );
   infFiles.push_back( mQCAlsoDeletePath);
   ULONG fileCount = (ULONG)infFiles.size();

   CString txt;
   DWORD flags = DRIVER_PACKAGE_DELETE_FILES | DRIVER_PACKAGE_FORCE;
   for (ULONG i = 0; i < fileCount; i++)
   {
      CString infFile = infFiles[i];

      BOOL bRebootFlag = FALSE;
      DWORD rc = mpFnUninstall( (LPCWSTR)infFile, flags, 0, &bRebootFlag );
      if ( (rc != ERROR_SUCCESS) 
      &&   (rc != ERROR_DRIVER_PACKAGE_NOT_IN_STORE)
      &&   (rc != ERROR_NO_DEVICE_ID) )
      {
         txt.Format( L"Uninstall( %s ) = %u", (LPCWSTR)infFile, rc );
         mLog.Log( (LPCWSTR)txt );

         /*return bRC;*/
      }
   }

   CString svcNameSer = L"qcusbser";
   if (mDriverTLA.GetLength() > 0)
   {
      svcNameSer += mDriverTLA;
   }

   CString svcNameNet = L"qcusbnet";
   if (mDriverTLA.GetLength() > 0)
   {
      svcNameNet += mDriverTLA;
   }
   
   CString svcNameNetWin7 = L"qcusbwwan";
   if (mDriverTLA.GetLength() > 0)
   {
      svcNameNetWin7 += mDriverTLA;
   }

   CString svcNameFilter = L"qcfilter";
   if (mDriverTLA.GetLength() > 0)
   {
      svcNameFilter += mDriverTLA;
   }

   /*svcNameSer += L"2k";
   svcNameNet += L"2k";
   svcNameFilter += L"2k";*/
#if 0   
   RemoveDriverService( (LPCWSTR)svcNameSer );
   if(true == biswin7)
   {
     RemoveDriverService( (LPCWSTR)svcNameNetWin7 );
   }
   else
   {
     RemoveDriverService( (LPCWSTR)svcNameNet );
   }
   RemoveDriverService( (LPCWSTR)svcNameFilter );
#endif

   // Delete the selective suspend registry key
   CString key = L"SYSTEM\\CurrentControlSet\\Services\\";
   key += svcNameSer;

   LPCWSTR pName = L"QCDriverSelectiveSuspendIdleTime";
   DeleteRegValue( (LPCWSTR)key, pName );

   ScanForHardwareChanges();

   bRC = true;
   return bRC;
}

/*===========================================================================
METHOD:
   WriteSelectSuspendRegVal (Public Method)

DESCRIPTION:
   Write the selective suspend registry value

PARAMETERS:
   val         [ I ] - DWORD value to write

RETURN VALUE:
   None
===========================================================================*/
void cDeviceManager::WriteSelectSuspendRegVal( DWORD val )
{
   if (mDeviceIDs.size() <= 0)
   {
      return;
   }

   CString key = L"SYSTEM\\CurrentControlSet\\Services\\qcusbser";
   if (mDriverTLA.GetLength() > 0)
   {
      key += mDriverTLA;
   }

   key += L"2k";

   LPCWSTR pName = L"QCDriverSelectiveSuspendIdleTime";
   WriteRegValue( (LPCWSTR)key, pName, val );
}

/*===========================================================================
METHOD:
   WriteTCPRegVals (Public Method)

DESCRIPTION:
   Write the hard-coded TCP registry values

RETURN VALUE:
   None
===========================================================================*/
void cDeviceManager::WriteTCPRegVals()
{
   if (mDeviceIDs.size() <= 0)
   {
      return;
   }

   LPCWSTR pKey = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters";

   std::map <CString, DWORD> keys;
   keys[CString( L"TcpWindowSize" )]          = mTCPWindowsSize;
   keys[CString( L"GlobalMaxTcpWindowSize" )] = mTCPMaxWindowsSize;
   keys[CString( L"Tcp1323Opts" )]            = mTCP123Opts;
   keys[CString( L"SackOpts" )]               = mTCPSackOpts;
   keys[CString( L"TcpMaxDupAcks" )]          = mTCPMaxDupAcks;
   keys[CString( L"TcpMaxConnectRetransmissions" )] 
      = mTCPMaxConnectRetransmissions;

   std::map <CString, DWORD>::const_iterator pIter = keys.begin();
   while (pIter != keys.end())
   {
      CString name = pIter->first;
      DWORD val = pIter->second;
      if (val != 0)
      {
         WriteRegValue( pKey, (LPCWSTR)name, val );
      }

      pIter++;
   }
}
