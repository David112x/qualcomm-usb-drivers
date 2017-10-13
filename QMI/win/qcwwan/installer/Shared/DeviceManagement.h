/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          DEVICEMANAGEMENT . H

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
#include "CfgMgr32.h"
#include "SetupAPI.h"
#include "DBT.h"
#include "DIFxAPI.h"

#include <map>
#include <set>
#define UNINSTALL_OLD 1/*change back after test*/
#ifndef NOCOINSTALLER
#define AUTO_CLOSE_UNSIGNED_WARN 1
#endif
// #define CALL_DRIVER_COINSTALLER 1
//#define DBG_ERROR_MESSAGE_BOX 1  /*Should be set to 0 in formal release*/

//
// exit codes
//
#define QC_OK      0
#define QC_REBOOT  1
#define QC_FAIL    2
#define QC_USAGE   3

struct QC_IdEnt {
    LPCTSTR QC_Str;     
    LPCTSTR QC_Wildchar;
    BOOL    QC_InstId;
};

//---------------------------------------------------------------------------
// Definitions
//---------------------------------------------------------------------------

// Device OEM names
extern LPCWSTR DEVICE_OEM_GENERIC;
extern LPCWSTR DEVICE_OEM_HP;
extern LPCWSTR DEVICE_OEM_LENOVO;
extern LPCWSTR DEVICE_OEM_ACER;
extern LPCWSTR DEVICE_OEM_SONY;
extern LPCWSTR DEVICE_OEM_TG;
extern LPCWSTR DEVICE_OEM_TOSHIBA;
extern LPCWSTR DEVICE_OEM_SAMSUNG;
extern LPCWSTR DEVICE_OEM_GD;
extern LPCWSTR DEVICE_OEM_ASUS;
extern LPCWSTR DEVICE_OEM_SIERRA;
extern LPCWSTR DEVICE_OEM_DIGI;
extern LPCWSTR DEVICE_OEM_DELL;
extern LPCWSTR DEVICE_OEM_PANASONIC;
extern LPCWSTR DEVICE_OEM_OPTION;

// DifxAPI function prototypes
typedef DWORD (WINAPI * pDifxPreInstall)( LPCWSTR, DWORD );
typedef DWORD (WINAPI * pDifxInstall)( LPCWSTR, DWORD, PVOID, BOOL * );
typedef DWORD (WINAPI * pDifxUninstall)( LPCWSTR, DWORD, PVOID, BOOL * );

/*=========================================================================*/
// Class cDeviceManagerLogger
//    Class to output logged information from device manager
/*=========================================================================*/
class cDeviceManagerLogger
{
   public:
      // (Inline) Constructor
      cDeviceManagerLogger()
      { };

      // (Inline) Destructor
      virtual ~cDeviceManagerLogger()
      { };

      // (Abstract) Log a message
      virtual void Log( LPCWSTR pMsg ) = 0;
};

/*=========================================================================*/
// Class cDeviceManager
//    Class to manage Gobi 2000 devices
/*=========================================================================*/
class cDeviceManager
{
   public:
      // Constructor
      cDeviceManager( 
         LPCWSTR                    pDeviceOEM,
         cDeviceManagerLogger &     logger );

      // (Inline) Destructor
      virtual ~cDeviceManager()
      {
         // Free the DLL module
         if (mhDLL != 0)
         {
            ::FreeLibrary( mhDLL );
            mhDLL = 0;
         }
      };

      // Is there a device present in the system?
      bool IsDevicePresent();

      // Rescan device tree for inactive devices
      void RescanDevices();

      // Remove all instances of the devices we support
      void RemoveDevices();

      // Install drivers
      bool InstallDrivers( 
         const CString &            path,
         bool                       b64,
         bool                       biswin7,
         bool                       bisLegacyEthernetDriver);

      // Uninstall drivers;
      bool UninstallDrivers( 
         const CString &            path,
         bool                       b64, 
         bool                       biswin7);
#if UNINSTALL_OLD
        bool UninstallPrevDrivers(const CString & infFilePath,const char *pDevId,const char *pCopyFileSection);
#endif      
	  // Write the selective suspend registry value
      void WriteSelectSuspendRegVal( DWORD val );

      // Write the hard-coded TCP registry values
      void WriteTCPRegVals();

   protected:
      // Enumerate devices for the given class
      std::map <DEVINST, CString> EnumerateDevices( 
         const GUID &               guid,
         bool                       bPresent = false );

      // Enumerate devices that have no driver loaded
      std::map <DEVINST, DEVINST> EnumerateDriverlessDevices();

      // Enumerate decendants of the given device with no driver loaded
      void EnumerateDriverlessDecendants( 
         DEVINST                             hParent,
         const std::map <DEVINST, CString> & devPresent,
         std::map <DEVINST, DEVINST> &       devMap );

      // Rescan the given device
      bool RescanDevice( LPCWSTR pDevID );

      // Remove the given device
      bool RemoveDevice( LPCWSTR pDevID );

      // Remove the service associated with the given driver 
      bool RemoveDriverService( LPCWSTR pService );

      // Setup the environment necessary for installing/unistalling drivers
      bool SetupDriverEnvironment( 
         const CString &            path,
         bool                       b64, 
         bool                       biswin7,
         bool                        bisLegacyEthernetDriver);

      // Delete a registry value
      bool DeleteRegValue(
         LPCWSTR                    pKey,
         LPCWSTR                    pName );

      // Write a registry value
      bool WriteRegValue(
         LPCWSTR                    pKey,
         LPCWSTR                    pName,
         DWORD                      val );

      /* Logger */
      cDeviceManagerLogger & mLog;

      /* Device IDs we service */
      std::set <CString> mDeviceIDs;

      /* Driver OEM */;
      LPCWSTR mpDriverOEM;

      /* Driver TLA */
      CString mDriverTLA;

      /* TCP/IP options */
      DWORD mTCPWindowsSize;
      DWORD mTCPMaxWindowsSize;
      DWORD mTCP123Opts;
      DWORD mTCPMaxConnectRetransmissions;
      DWORD mTCPSackOpts;
      DWORD mTCPMaxDupAcks;

      /* DifxAPI DLL handle */
      HMODULE mhDLL;

      /* DifxAPI function pointers */
      pDifxPreInstall mpFnPreinstall;
      pDifxInstall mpFnInstall;
      pDifxUninstall mpFnUninstall;

      /* Paths to driver files */
      CString mQCSerialPath;
      CString mQCModemPath;
      CString mQCNetPath;
      CString mQCAlsoDeletePath;
      CString mQCFilterPath;
#if CALL_DRIVER_COINSTALLER	  
	  CString mQCCoinstallerPath;
	  CString mQCCoinstallerDllPath;
#endif	  
#if AUTO_CLOSE_UNSIGNED_WARN	  
	  CString mQCSetupApiDllPath;
#endif
      CString mQCQdssPath;
};
