/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             Q C P N P . C

GENERAL DESCRIPTION
  This file implementations of PNP functions.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
//  Copyright 3Com Corporation 1997   All Rights Reserved.
//
                   
#include <stdarg.h>
#include <stdio.h>
#include "QCMAIN.h"
#include "QCPTDO.h"
#include "QCUTILS.h"
#include "QCINT.h"
#include "QCSER.h"
#include "QCPNP.h"
#include "QCDSP.h"
#include "QCRD.h"
#include "QCWT.h"
#include "QCPWR.h"
#include "QCDEV.h"

#ifdef EVENT_TRACING
#include "QCPNP.tmh"      //  this is the file that will be auto generated
#endif

extern NTKERNELAPI VOID IoReuseIrp(IN OUT PIRP Irp, IN NTSTATUS Iostatus);

//{0x98b06a49, 0xb09e, 0x4896, {0x94, 0x46, 0xd9, 0x9a, 0x28, 0xca, 0x4e, 0x5d}};
GUID gQcFeatureGUID = {0x496ab098, 0x9eb0, 0x9648,  // in network byte order
                       {0x94, 0x46, 0xd9, 0x9a, 0x28, 0xca, 0x4e, 0x5d}};

// #ifdef DEBUG_MSGS
static const PCHAR szSystemPowerState[] = 
{
   "PowerSystemUnspecified",
   "PowerSystemWorking",
   "PowerSystemSleeping1",
   "PowerSystemSleeping2",
   "PowerSystemSleeping3",
   "PowerSystemHibernate",
   "PowerSystemShutdown",
   "PowerSystemMaximum"
};

static const PCHAR szDevicePowerState[] = 
{
   "PowerDeviceUnspecified",
   "PowerDeviceD0",
   "PowerDeviceD1",
   "PowerDeviceD2",
   "PowerDeviceD3",
   "PowerDeviceMaximum"
};
// #endif // DEBUG_MSGS

NTSTATUS QCPNP_AddDevice
(
   IN PDRIVER_OBJECT pDriverObject,
   IN PDEVICE_OBJECT pdo
)
{
   char                  myPortName[16];
   NTSTATUS              ntStatus  = STATUS_SUCCESS, nts;
   PDEVICE_OBJECT        fdo = NULL, myPortDO = NULL, stackTopDO;
   PDEVICE_EXTENSION     portDoExt = NULL;
   PFDO_DEVICE_EXTENSION pDevExt;
   ULONG                 reqLen;
   POWER_STATE           initialPwrState;
   #ifdef QCSER_TARGET_XP
   KLOCK_QUEUE_HANDLE levelOrHandle;
   #else
   KIRQL levelOrHandle;
   #endif

   QcAcquireSpinLock(&gPnpSpinLock, &levelOrHandle);
   if (gPnpState == QC_PNP_REMOVING)
   {
      QCSER_DbgPrintG
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_ERROR,
         ("<%s> AddDevice: ERR: Removal in progress QC_PNP_REMOVING\n", gDeviceName)
      );
      //QcReleaseSpinLock(&gPnpSpinLock, levelOrHandle);
      //return STATUS_DELETE_PENDING;
   }
   gPnpState = QC_PNP_ADDING;
   QcReleaseSpinLock(&gPnpSpinLock, levelOrHandle);

   QcAcquireEntryPass(&gSyncEntryEvent, "qc-add");

   ntStatus = QCSER_VendorRegistryProcess(pDriverObject, pdo);
   if (!NT_SUCCESS(ntStatus))
   {
      DbgPrint("<%s> AddDevice: reg access failure.\n", gDeviceName);
      goto QCPNP_PnPAddDevice_Return;
   }

   QCSER_DbgPrintG
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_DETAIL,
      ("<%s> AddDevice: <%d+%d>\n   DRVO=0x%p, PHYDEVO=0x%p\n", gDeviceName,
          sizeof(struct _DEVICE_OBJECT), sizeof(DEVICE_EXTENSION),
          pDriverObject, pdo)
   );

   myPortDO = QCPTDO_Create(pDriverObject, pdo);

   if (myPortDO == NULL)
   {
      QCSER_DbgPrintG
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_CRITICAL,
         ("<%s> AddDevice: ERR-0\n", gDeviceName)
      );
      ntStatus = STATUS_UNSUCCESSFUL;
      goto QCPNP_PnPAddDevice_Return;
   } 

   portDoExt = (PDEVICE_EXTENSION)myPortDO->DeviceExtension;
   strcpy(myPortName, portDoExt->PortName);

   portDoExt->FdoDeviceType = QCPNP_GetDeviceType(myPortDO);

   // we got myPortDO, now create FDO
   QCSER_DbgPrintG
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_DETAIL,
      ("<%s> AddDevice: Creating FDO...\n", myPortName)
   );

   ntStatus = IoCreateDevice
              (
                 pDriverObject,
                 sizeof(FDO_DEVICE_EXTENSION),
                 NULL,                          // unnamed
                 portDoExt->FdoDeviceType,      // FILE_DEVICE_UNKNOWN, _SERIAL_PORT, _MODEM
                 0, // FILE_DEVICE_SECURE_OPEN, // DeviceCharacteristics
                 FALSE,
                 &fdo
              );

   if (NT_SUCCESS(ntStatus))
   {
      QCSER_DbgPrintG
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_DETAIL,
         ("<%s> AddDevice: new FDO 0x%p\n", myPortName, fdo)
      );
      fdo->Flags |= DO_DIRECT_IO;
      fdo->Flags &= ~DO_EXCLUSIVE;
   }
   else
   {
      QCSER_DbgPrintG
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_CRITICAL,
         ("<%s> AddDevice: FDO failure 0x%x\n", myPortName, ntStatus)
      );
      fdo = NULL;
      goto QCPNP_PnPAddDevice_Return;
   }

   pDevExt = (PFDO_DEVICE_EXTENSION)fdo->DeviceExtension;

   pDevExt->StackDeviceObject = IoAttachDeviceToDeviceStack(fdo, pdo);
   if (pDevExt->StackDeviceObject != NULL)
   {
      QCSER_AddToFdoCollection(portDoExt, fdo);
      QCSER_DbgPrintG
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_DETAIL,
         ("<%s> AddDevice: Attached: lrPtr=0x%p upPtr=0x%p\n", myPortName,
          pDevExt->StackDeviceObject,
          pDevExt->StackDeviceObject->AttachedDevice)
      );
   }
   else
   {
      QCSER_DbgPrintG
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_CRITICAL,
         ("<%s> AddDevice: IoAttachDeviceToDeviceStack failure\n", myPortName)
      );
      ntStatus = STATUS_UNSUCCESSFUL;
      goto QCPNP_PnPAddDevice_Return;
   }

   // set our DO's StackSize
   stackTopDO = IoGetAttachedDeviceReference(fdo);
   myPortDO->StackSize = stackTopDO->StackSize + 2;
   ObDereferenceObject(stackTopDO);
   QCSER_DbgPrintG
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_ERROR,
      ("<%s> AddDevice: StackSize %u/%u:%u\n", myPortName,
        fdo->StackSize, myPortDO->StackSize, stackTopDO->StackSize)
   );

   // Initialize device extension
   pDevExt->PDO = pdo;
   pDevExt->MyDeviceObject = fdo;
   pDevExt->PortDevice = myPortDO;

   // Continue initializing extension of the port DO
   portDoExt->PhysicalDeviceObject = pDevExt->PDO;
   portDoExt->StackDeviceObject = pDevExt->StackDeviceObject;
   portDoExt->FDO = fdo;

   portDoExt->bDeviceRemoved = FALSE;
   portDoExt->bDeviceSurpriseRemoved = FALSE;
   portDoExt->bmDevState = DEVICE_STATE_ZERO;

   QCPNP_GetDeviceCapabilities(portDoExt, TRUE); // store info in portExt

   if(pdo->Flags & DO_POWER_PAGABLE)
   {
      fdo->Flags |= DO_POWER_PAGABLE;
   }
   fdo->Flags &= ~DO_DEVICE_INITIALIZING;

   // QCPNP_RegisterDeviceInterface(myPortDO);

   // Initialize L2 Buffers
   ntStatus = QCRD_InitializeL2Buffers(portDoExt);
   if (!NT_SUCCESS(ntStatus))
   {
     QCSER_DbgPrintG
     (
        QCSER_DBG_MASK_CONTROL,
        QCSER_DBG_LEVEL_FORCE,
        ("<%s> QCPNP: L2 NO MEM\n", myPortName)
     );
     goto QCPNP_PnPAddDevice_Return;
   }

   #ifdef QCUSB_MULTI_WRITES
   ntStatus = QCMWT_InitializeMultiWriteElements(portDoExt);
   if (!NT_SUCCESS(ntStatus))
   {
     QCSER_DbgPrintG
     (
        QCSER_DBG_MASK_CONTROL,
        QCSER_DBG_LEVEL_FORCE,
        ("<%s> QCPNP: MWT NO MEM\n", myPortName)
     );
     goto QCPNP_PnPAddDevice_Return;
   }
   #endif // QCUSB_MULTI_WRITES

   portDoExt->SystemPower = PowerSystemWorking;
   portDoExt->DevicePower = initialPwrState.DeviceState = PowerDeviceD0;
   PoSetPowerState(fdo, DevicePowerState, initialPwrState);

   ntStatus = InitDispatchThread(myPortDO);

QCPNP_PnPAddDevice_Return:

   QcReleaseEntryPass(&gSyncEntryEvent, "qc-add", "END");

   QcAcquireSpinLock(&gPnpSpinLock, &levelOrHandle);
   gPnpState = QC_PNP_IDLE;
   QcReleaseSpinLock(&gPnpSpinLock, levelOrHandle);

   if (NT_SUCCESS(ntStatus))
   {
      QCSER_DbgPrintG
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_FORCE,
         ("<%s> QCPNP: ReportDeviceName (NT 0x%x) type %d/%d\n", myPortName, ntStatus, portDoExt->FdoDeviceType, FILE_DEVICE_SERIAL_PORT)
      );
      initDevStateX(portDoExt, DEVICE_STATE_PRESENT);

      if (portDoExt->FdoDeviceType == FILE_DEVICE_SERIAL_PORT)
      {
         QCPNP_ReportDeviceName(portDoExt);
      }

   }
   else if (portDoExt != NULL)
   {
      QCSER_CleanupDeviceExtensionBuffers(myPortDO);

      // cleanup device objects
      if (fdo != NULL)
      {
         IoDeleteDevice(fdo);
      }
      if (myPortDO != NULL)
      {
         IoDeleteDevice(myPortDO);
      }
   }

   return ntStatus;
}  // AddDevice

//  Copyright 3Com Corporation 1997   All Rights Reserved.
//
//
//
//  09/29/97  Tim  Added Device Interface 'shingle' for CCPORT interface
//
//

NTSTATUS QCPNP_GetDeviceCapabilities
(
   PDEVICE_EXTENSION deviceExtension,
   BOOLEAN bPowerManagement
)
{
   NTSTATUS ntStatus;
   PIRP     pIrp;
   KEVENT   syncEvent;
   ULONG    i;
   PIO_STACK_LOCATION pStackLocation;

   // Get a copy of the physical device's capabilities into a
   // DEVICE_CAPABILITIES struct in our device extension;
   // We are most interested in learning which system power states
   // are to be mapped to which device power states for handling
   // IRP_MJ_SET_POWER Irps.

   KeInitializeEvent(&syncEvent, NotificationEvent, FALSE);
   RtlZeroMemory(&(deviceExtension->DeviceCapabilities), sizeof(DEVICE_CAPABILITIES));
   if ((pIrp = IoAllocateIrp(deviceExtension->PhysicalDeviceObject->StackSize, FALSE)) == NULL)
   {
      return STATUS_NO_MEMORY;
   }
   else
   {
      deviceExtension->DeviceCapabilities.Version  = 1;
      deviceExtension->DeviceCapabilities.Size     = sizeof (DEVICE_CAPABILITIES);
      deviceExtension->DeviceCapabilities.Address  = 0xffffffff;
      deviceExtension->DeviceCapabilities.UINumber = 0xffffffff;
      deviceExtension->DeviceCapabilities.SurpriseRemovalOK = TRUE;
      deviceExtension->DeviceCapabilities.Removable = FALSE;
      pStackLocation = IoGetNextIrpStackLocation(pIrp);
      pStackLocation->MajorFunction= IRP_MJ_PNP;
      pStackLocation->MinorFunction= IRP_MN_QUERY_CAPABILITIES;;
      pStackLocation->Parameters.DeviceCapabilities.Capabilities = &(deviceExtension->DeviceCapabilities);

      // PNP IRP must have its status initialized to STATUS_NOT_SUPPORTED
      pIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;
   }

   IoSetCompletionRoutine(pIrp, QCUSB_CallUSBD_Completion, &syncEvent, TRUE, TRUE, TRUE);

   ntStatus = IoCallDriver(deviceExtension->PhysicalDeviceObject, pIrp);

   if (ntStatus == STATUS_PENDING)
   {
     // make it sync call
      ntStatus = KeWaitForSingleObject(&syncEvent, Suspended, KernelMode, FALSE, NULL);
   }

   // We want to determine what level to auto-powerdown to; This is the lowest
   // sleeping level that is LESS than D3; 
   // If all are set to D3, auto powerdown/powerup will be disabled.

   // init to disabled
   deviceExtension->PowerDownLevel = PowerDeviceUnspecified;

   for (i=PowerSystemUnspecified; i< PowerSystemMaximum; i++)
   {
      if (deviceExtension->DeviceCapabilities.DeviceState[i] < PowerDeviceD3)
      {
         deviceExtension->PowerDownLevel = deviceExtension->DeviceCapabilities.DeviceState[i];
      }
   }

   if ((deviceExtension->PowerDownLevel == PowerDeviceUnspecified) ||
       (deviceExtension->PowerDownLevel <= PowerDeviceD0))
   {
       deviceExtension->PowerDownLevel = PowerDeviceD2;
   }

   deviceExtension->DeviceCapabilities.D1Latency  = 4000;  // 0.4 second
   deviceExtension->DeviceCapabilities.D2Latency  = 5000;  // 0.5 second

   QCPWR_VerifyDeviceCapabilities(deviceExtension);

   IoFreeIrp(pIrp);

   QCSER_DbgPrintG
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_DETAIL,
      ("WakeFromD0/1/2/3 = %u, %u, %u, %u\n",
          deviceExtension->DeviceCapabilities.WakeFromD0,
          deviceExtension->DeviceCapabilities.WakeFromD1,
          deviceExtension->DeviceCapabilities.WakeFromD2,
          deviceExtension->DeviceCapabilities.WakeFromD3
      )
   );

   QCSER_DbgPrintG
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_DETAIL,
      ("SystemWake = %s\n",
       QCPNP_StringForSysState
       (
          deviceExtension->DeviceCapabilities.SystemWake
       )
      )
   );
   QCSER_DbgPrintG
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_DETAIL,
      ("DeviceWake = %s\n",
         QCPNP_StringForDevState
         (
            deviceExtension->DeviceCapabilities.DeviceWake
         )
      )
   );

   for (i=PowerSystemUnspecified; i< PowerSystemMaximum; i++)
   {
      QCSER_DbgPrintG
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_DETAIL,
         ("Device State Map: sysstate<%s> = devstate<%s>\n",
          QCPNP_StringForSysState(i),
          QCPNP_StringForDevState
          (
             deviceExtension->DeviceCapabilities.DeviceState[i]
          )
         )
      );
   }

   QCSER_DbgPrintG
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_DETAIL,
      ("D1-D3 Latency = %u, %u, %u (x 100us)\n",
         deviceExtension->DeviceCapabilities.D1Latency,
         deviceExtension->DeviceCapabilities.D2Latency,
         deviceExtension->DeviceCapabilities.D3Latency
      )
   );

   return ntStatus;
}

NTSTATUS QCSER_VendorRegistryProcess
(
   IN PDRIVER_OBJECT pDriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject
)
{
   NTSTATUS       ntStatus  = STATUS_SUCCESS;
   UNICODE_STRING ucDriverVersion;
   HANDLE         hRegKey = NULL;
   UNICODE_STRING ucValueName;
   ULONG          ulWriteUnit = 0;
   ULONG          gDriverConfigParam = 0;

   // Init default configuration
   gVendorConfig.ContinueOnOverflow  = FALSE;
   gVendorConfig.ContinueOnOverflow  = FALSE;
   gVendorConfig.ContinueOnDataError = FALSE;
   gVendorConfig.Use128ByteInPkt     = FALSE;
   gVendorConfig.Use256ByteInPkt     = FALSE;
   gVendorConfig.Use512ByteInPkt     = FALSE;
   gVendorConfig.Use1024ByteInPkt    = FALSE;
   gVendorConfig.Use2048ByteInPkt    = FALSE;
   gVendorConfig.Use1kByteOutPkt     = FALSE;
   gVendorConfig.Use2kByteOutPkt     = FALSE;
   gVendorConfig.Use4kByteOutPkt     = FALSE;
   gVendorConfig.Use128KReadBuffer   = FALSE;
   gVendorConfig.Use256KReadBuffer   = FALSE;
   gVendorConfig.NoTimeoutOnCtlReq   = FALSE;
   gVendorConfig.RetryOnTxError      = FALSE;
   gVendorConfig.UseReadArray        = TRUE;
   gVendorConfig.UseMultiWrites      = TRUE;
   gVendorConfig.EnableLogging       = FALSE;
   gVendorConfig.MinInPktSize        = 64;
   gVendorConfig.MaxPipeXferSize     = QCSER_RECEIVE_BUFFER_SIZE*8;
   gVendorConfig.WriteUnitSize       = USB_WRITE_UNIT_SIZE;
   gVendorConfig.InternalReadBufSize = USB_INTERNAL_READ_BUFFER_SIZE;
   gVendorConfig.LoggingWriteThrough = FALSE;
   gVendorConfig.NumOfRetriesOnError = BEST_RETRIES;
   gVendorConfig.LogLatestPkts       = FALSE;

   // Update driver version in the registry
   ntStatus = IoOpenDeviceRegistryKey
              (
                 PhysicalDeviceObject,
                 PLUGPLAY_REGKEY_DRIVER,
                 KEY_ALL_ACCESS,
                 &hRegKey
              );
   if (!NT_SUCCESS(ntStatus))
   {
      return ntStatus;
   }
   ucHdlCnt++;

   _zeroUnicode(ucDriverVersion);
   RtlInitUnicodeString(&ucValueName, VEN_DEV_VER);
   ntStatus = getRegValueEntryData
              (
                 hRegKey, ucValueName.Buffer, &ucDriverVersion
              );
   if (ntStatus == STATUS_SUCCESS)
   {
      ANSI_STRING ansiStr;
      RtlUnicodeStringToAnsiString(&ansiStr, &ucDriverVersion, TRUE);
      strcpy(gVendorConfig.DriverVersion, ansiStr.Buffer);
      RtlFreeAnsiString(&ansiStr);
   }

   // Get number of retries on error
   RtlInitUnicodeString(&ucValueName, VEN_DEV_RTY_NUM);
   ntStatus = getRegDwValueEntryData
              (
                 hRegKey,
                 ucValueName.Buffer,
                 &gVendorConfig.NumOfRetriesOnError
              );

   if (ntStatus != STATUS_SUCCESS)
   {
      gVendorConfig.NumOfRetriesOnError = BEST_RETRIES;
   }
   else
   {
      if (gVendorConfig.NumOfRetriesOnError < BEST_RETRIES_MIN)
      {
         gVendorConfig.NumOfRetriesOnError = BEST_RETRIES_MIN;
      }
      else if (gVendorConfig.NumOfRetriesOnError > BEST_RETRIES_MAX)
      {
         gVendorConfig.NumOfRetriesOnError = BEST_RETRIES_MAX;
      }
   }

   // Get Max pipe transfer size
   RtlInitUnicodeString(&ucValueName, VEN_DEV_MAX_XFR);
   ntStatus = getRegDwValueEntryData
              (
                 hRegKey,
                 ucValueName.Buffer,
                 &gVendorConfig.MaxPipeXferSize
              );

   if (ntStatus != STATUS_SUCCESS)
   {
      gVendorConfig.MaxPipeXferSize = QCSER_RECEIVE_BUFFER_SIZE*8;
   }

   // Get number of L2 buffers
   RtlInitUnicodeString(&ucValueName, VEN_DEV_L2_BUFS);
   ntStatus = getRegDwValueEntryData
              (
                 hRegKey,
                 ucValueName.Buffer,
                 &gVendorConfig.NumberOfL2Buffers
              );

   if (ntStatus != STATUS_SUCCESS)
   {
      gVendorConfig.NumberOfL2Buffers = QCSER_NUM_OF_LEVEL2_BUF;
   }
   else
   {
      if (gVendorConfig.NumberOfL2Buffers < 2)
      {
         gVendorConfig.NumberOfL2Buffers = 2;
      }
      else if (gVendorConfig.NumberOfL2Buffers > QCUSB_MAX_MRW_BUF_COUNT)
      {
         gVendorConfig.NumberOfL2Buffers = QCUSB_MAX_MRW_BUF_COUNT;
      }
   }

   // Get Debug Level
   RtlInitUnicodeString(&ucValueName, VEN_DBG_MASK);
   ntStatus = getRegDwValueEntryData
              (
                 hRegKey,
                 ucValueName.Buffer,
                 &gVendorConfig.DebugMask
              );

   if (ntStatus != STATUS_SUCCESS)
   {
      gVendorConfig.DebugMask = QCSER_DBG_LEVEL_FORCE;
   }
   #ifdef DEBUG_MSGS
   gVendorConfig.DebugMask = 0xFFFFFFFF;
   #endif
   gVendorConfig.DebugLevel = (UCHAR)(gVendorConfig.DebugMask & 0x0F);

   // Get driver write unit
   RtlInitUnicodeString(&ucValueName, VEN_DRV_WRITE_UNIT);
   ntStatus = getRegDwValueEntryData
              (
                 hRegKey,
                 ucValueName.Buffer,
                 &ulWriteUnit
              );

   if (ntStatus != STATUS_SUCCESS)
   {
      ulWriteUnit = 0;
   }

   gVendorConfig.MaxPipeXferSize = gVendorConfig.MaxPipeXferSize / 64 * 64;
   if (gVendorConfig.MaxPipeXferSize > 4096*8)
   {
      gVendorConfig.MaxPipeXferSize = 4096*8;
   }
   if (gVendorConfig.MaxPipeXferSize < 1024)
   {
      gVendorConfig.MaxPipeXferSize = 1024;
   }

   // Get config parameter
   RtlInitUnicodeString(&ucValueName, VEN_DEV_CONFIG);
   ntStatus = getRegDwValueEntryData
              (
                 hRegKey, ucValueName.Buffer, &gDriverConfigParam
              );

   if (ntStatus != STATUS_SUCCESS)
   {
      gDriverConfigParam = 0;
   }
   else
   {
      if (gDriverConfigParam & QCSER_CONTINUE_ON_OVERFLOW)
      {
         gVendorConfig.ContinueOnOverflow = TRUE;
      }
      if (gDriverConfigParam & QCSER_CONTINUE_ON_DATA_ERR)
      {
         gVendorConfig.ContinueOnDataError = TRUE;
      }
      if (gDriverConfigParam & QCSER_USE_128_BYTE_IN_PKT)
      {
         gVendorConfig.Use128ByteInPkt = TRUE;
         gVendorConfig.MinInPktSize    = 128;
      }
      if (gDriverConfigParam & QCSER_USE_256_BYTE_IN_PKT)
      {
         gVendorConfig.Use256ByteInPkt = TRUE;
         gVendorConfig.MinInPktSize    = 256;
      }
      if (gDriverConfigParam & QCSER_USE_512_BYTE_IN_PKT)
      {
         gVendorConfig.Use512ByteInPkt = TRUE;
         gVendorConfig.MinInPktSize    = 512;
      }
      if (gDriverConfigParam & QCSER_USE_1024_BYTE_IN_PKT)
      {
         gVendorConfig.Use1024ByteInPkt = TRUE;
         gVendorConfig.MinInPktSize    = 1024;
      }
      if (gDriverConfigParam & QCSER_USE_2048_BYTE_IN_PKT)
      {
         gVendorConfig.Use2048ByteInPkt = TRUE;
         gVendorConfig.MinInPktSize    = 2048;
      }
      if (gDriverConfigParam & QCSER_USE_1K_BYTE_OUT_PKT)
      {
         gVendorConfig.Use1kByteOutPkt   = TRUE;
         gVendorConfig.WriteUnitSize     = 1024;
      }
      if (gDriverConfigParam & QCSER_USE_2K_BYTE_OUT_PKT)
      {
         gVendorConfig.Use2kByteOutPkt = TRUE;
         gVendorConfig.WriteUnitSize    = 2048;
      }
      if (gDriverConfigParam & QCSER_USE_4K_BYTE_OUT_PKT)
      {
         gVendorConfig.Use4kByteOutPkt = TRUE;
         gVendorConfig.WriteUnitSize    = 4096;
      }
      if (gDriverConfigParam & QCSER_USE_128K_READ_BUFFER)
      {
         gVendorConfig.Use128KReadBuffer   = TRUE;
         gVendorConfig.InternalReadBufSize = 128 * 1024L;
      }
      if (gDriverConfigParam & QCSER_USE_256K_READ_BUFFER)
      {
         gVendorConfig.Use256KReadBuffer   = TRUE;
         gVendorConfig.InternalReadBufSize = 256 * 1024L;
      }
      if (gDriverConfigParam & QCSER_NO_TIMEOUT_ON_CTL_REQ)
      {
         gVendorConfig.NoTimeoutOnCtlReq = TRUE;
      }
      if (gDriverConfigParam & QCSER_ENABLE_LOGGING)
      {
         gVendorConfig.EnableLogging = TRUE;
      }
      if (gDriverConfigParam & QCSER_RETRY_ON_TX_ERROR)
      {
         gVendorConfig.RetryOnTxError = TRUE;
      }
      // if ((gDriverConfigParam & QCSER_USE_READ_ARRAY) == 0)
      // {
      //    gVendorConfig.UseReadArray = FALSE;
      // }
      // if ((gDriverConfigParam & QCSER_USE_MULTI_WRITES) == 0)
      // {
      //    gVendorConfig.UseMultiWrites = FALSE;
      // }
      if (gDriverConfigParam & QCSER_LOG_LATEST_PKTS)
      {
         gVendorConfig.LogLatestPkts = TRUE;
      }
      if (gDriverConfigParam & QCSER_LOGGING_WRITE_THROUGH)
      {
         gVendorConfig.LoggingWriteThrough = TRUE;
      }
   }

   if (ulWriteUnit > 64)
   {
      gVendorConfig.WriteUnitSize = ulWriteUnit;
   }
   gVendorConfig.WriteUnitSize = gVendorConfig.WriteUnitSize / 64 * 64;
   if (gVendorConfig.WriteUnitSize < 64)
   {
      gVendorConfig.WriteUnitSize = 64;
   }
   if (gVendorConfig.MaxPipeXferSize < gVendorConfig.MinInPktSize)
   {
      gVendorConfig.MaxPipeXferSize = gVendorConfig.MinInPktSize;
   }

   _closeRegKeyG( gDeviceName, hRegKey, "PNP-2" );
   _freeString(ucDriverVersion);

   if (gVendorConfig.DebugLevel > 0)
   {
      DbgPrint("<%s> Vendor Config Info ------ \n", gDeviceName);
      DbgPrint("    ContinueOnOverflow:  0x%x\n", gVendorConfig.ContinueOnOverflow);
      DbgPrint("    ContinueOnDataError: 0x%x\n", gVendorConfig.ContinueOnDataError);
      DbgPrint("    Use128ByteInPkt:     0x%x\n", gVendorConfig.Use128ByteInPkt);
      DbgPrint("    Use256ByteInPkt:     0x%x\n", gVendorConfig.Use256ByteInPkt);
      DbgPrint("    Use512ByteInPkt:     0x%x\n", gVendorConfig.Use512ByteInPkt);
      DbgPrint("    Use1024ByteInPkt:    0x%x\n", gVendorConfig.Use1024ByteInPkt);
      DbgPrint("    Use2048ByteInPkt:    0x%x\n", gVendorConfig.Use2048ByteInPkt);
      DbgPrint("    Use1kByteOutPkt:     0x%x\n", gVendorConfig.Use1kByteOutPkt);
      DbgPrint("    Use2kByteOutPkt:     0x%x\n", gVendorConfig.Use2kByteOutPkt);
      DbgPrint("    Use4kByteOutPkt:     0x%x\n", gVendorConfig.Use4kByteOutPkt);
      DbgPrint("    Use128KReadBuffer:   0x%x\n", gVendorConfig.Use128KReadBuffer);
      DbgPrint("    Use256KReadBuffer:   0x%x\n", gVendorConfig.Use256KReadBuffer);
      DbgPrint("    NoTimeoutOnCtlReq:   0x%x\n", gVendorConfig.NoTimeoutOnCtlReq);
      DbgPrint("    EnableLogging:       0x%x\n", gVendorConfig.EnableLogging);
      DbgPrint("    RetryOnTxError       0x%x\n", gVendorConfig.RetryOnTxError);
      DbgPrint("    UseReadArray         0x%x\n", gVendorConfig.UseReadArray);
      DbgPrint("    UseMultiWrites       0x%x\n", gVendorConfig.UseMultiWrites);
      DbgPrint("    LogLatestPkts        0x%x\n", gVendorConfig.LogLatestPkts);
      DbgPrint("    MinInPktSize:        %ld\n",  gVendorConfig.MinInPktSize);
      DbgPrint("    MaxPipeXferSize      %ld\n",  gVendorConfig.MaxPipeXferSize);
      DbgPrint("    WriteUnitSize:       %ld\n",  gVendorConfig.WriteUnitSize);
      DbgPrint("    InternalReadBufSize: %ldK\n", gVendorConfig.InternalReadBufSize/1024);
      DbgPrint("    NumOfRetriesOnError: %ld\n",  gVendorConfig.NumOfRetriesOnError);
      DbgPrint("    NumberOfL2Buffers:   %ld\n",  gVendorConfig.NumberOfL2Buffers);
      DbgPrint("    LoggingWriteThrough: %ld\n",  gVendorConfig.LoggingWriteThrough);
      DbgPrint("    DebugMask:           0x%x\n", gVendorConfig.DebugMask);
      DbgPrint("    DebugLevel:          %ld\n",  gVendorConfig.DebugLevel);
   }


   return STATUS_SUCCESS;
} // QCSER_VendorRegistryProcess

NTSTATUS QCSER_PostVendorRegistryProcess
(
   IN PDRIVER_OBJECT pDriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject,
   IN PDEVICE_OBJECT DeviceObject
)
{
   NTSTATUS       ntStatus  = STATUS_SUCCESS;
   HANDLE         hRegKey = NULL;
   UNICODE_STRING ucValueName;
   PDEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;
   ULONG          ulValue = 0;

   pDevExt->bLoggingOk = FALSE;

   _zeroUnicode(pDevExt->ucLoggingDir);

   ntStatus = IoOpenDeviceRegistryKey
              (
                 PhysicalDeviceObject,
                 PLUGPLAY_REGKEY_DRIVER,
                 KEY_ALL_ACCESS,
                 &hRegKey
              );
   if (!NT_SUCCESS(ntStatus))
   {
      // _freeString(pDevExt->ucLoggingDir);
      return ntStatus;
   }
   ucHdlCnt++;

   RtlInitUnicodeString(&ucValueName, VEN_DEV_LOG_DIR);
   ntStatus = getRegValueEntryData
              (
                 hRegKey, ucValueName.Buffer, &pDevExt->ucLoggingDir
              );
   if (ntStatus == STATUS_SUCCESS)
   {
      pDevExt->bLoggingOk = TRUE;
      _dbgPrintUnicodeString(&pDevExt->ucLoggingDir,"pDevExt->ucLoggingDir");

   }

   // Write port info for user to locate the correct registry location
   RtlInitUnicodeString(&ucValueName, VEN_DEV_PORT);
   {
      ZwSetValueKey
      (
         hRegKey,
         &ucValueName,
         0,
         REG_SZ,
         pDevExt->ucsPortName.Buffer,
         pDevExt->ucsPortName.Length+2 // include terminating zeros
      );
   }

   // Get device function
   RtlInitUnicodeString(&ucValueName, VEN_DRV_DEV_FUNC);
   ntStatus = getRegDwValueEntryData
              (
                 hRegKey,
                 ucValueName.Buffer,
                 &ulValue
              );
   pDevExt->DeviceFunction = QCUSB_DEV_FUNC_UNDEF;
   if (ntStatus == STATUS_SUCCESS)
   {
      pDevExt->DeviceFunction = ulValue;
   }

   #ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif // QCUSB_MUX_PROTOCOL

   // Aggregation enabled
   RtlInitUnicodeString(&ucValueName, VEN_DRV_AGG_ON);
   ntStatus = getRegDwValueEntryData
              (
                 hRegKey,
                 ucValueName.Buffer,
                 &(pDevExt->AggregationEnabled)
              );

   if (ntStatus != STATUS_SUCCESS)
   {
      pDevExt->AggregationEnabled = 0;
   }
   else
   {
      if (pDevExt->AggregationEnabled != 1)
      {
         pDevExt->AggregationEnabled = 0;
      }
   }

   // aggregation time/size
   RtlInitUnicodeString(&ucValueName, VEN_DRV_AGG_TIME);
   ntStatus = getRegDwValueEntryData
              (
                 hRegKey,
                 ucValueName.Buffer,
                 &(pDevExt->AggregationPeriod)
              );

   if (ntStatus != STATUS_SUCCESS)
   {
      pDevExt->AggregationPeriod = QCUSB_AGGREGATION_PERIOD_DEFAULT;  // 2ms
   }
   else
   {
      if (pDevExt->AggregationPeriod > QCUSB_AGGREGATION_PERIOD_MAX)
      {
         pDevExt->AggregationPeriod = QCUSB_AGGREGATION_PERIOD_MAX;  // 5ms
      }
      if (pDevExt->AggregationPeriod < QCUSB_AGGREGATION_PERIOD_MIN)
      {
         pDevExt->AggregationPeriod = QCUSB_AGGREGATION_PERIOD_MIN;   // .5ms
      }
   }

   RtlInitUnicodeString(&ucValueName, VEN_DRV_AGG_SIZE);
   ntStatus = getRegDwValueEntryData
              (
                 hRegKey,
                 ucValueName.Buffer,
                 &(pDevExt->WriteAggBuffer.MaxSize)
              );
   if (ntStatus != STATUS_SUCCESS)
   {
      pDevExt->WriteAggBuffer.MaxSize = QCUSB_AGGREGATION_SIZE_DEFAULT;
   }
   else
   {
      if (pDevExt->WriteAggBuffer.MaxSize > QCUSB_AGGREGATION_SIZE_MAX)
      {
         pDevExt->WriteAggBuffer.MaxSize = QCUSB_AGGREGATION_SIZE_MAX;
      }
      if (pDevExt->WriteAggBuffer.MaxSize < QCUSB_AGGREGATION_SIZE_MIN)
      {
         pDevExt->WriteAggBuffer.MaxSize = QCUSB_AGGREGATION_SIZE_MIN;
      }
   }

   // aggregation time/size
	RtlInitUnicodeString(&ucValueName, VEN_DRV_RES_TIME);
	ntStatus = getRegDwValueEntryData
			   (
				  hRegKey,
				  ucValueName.Buffer,
				  &(pDevExt->ResolutionTime)
			   );
   
	if (ntStatus != STATUS_SUCCESS)
	{
	   pDevExt->ResolutionTime = QCUSB_RESOLUTION_TIME_DEFAULT;  // 1ms
	}
	else
	{
	   if (pDevExt->ResolutionTime > QCUSB_RESOLUTION_TIME_MAX)
	   {
		  pDevExt->ResolutionTime = QCUSB_RESOLUTION_TIME_MAX;  // 5ms
	   }
	   if (pDevExt->ResolutionTime <= QCUSB_RESOLUTION_TIME_MIN)
	   {
		  pDevExt->ResolutionTime = QCUSB_RESOLUTION_TIME_MIN;   // 1ms
	   }
	}

   _closeRegKey( hRegKey, "PNP-3" );

   return STATUS_SUCCESS;
}  // QCSER_PostVendorRegistryProcess

VOID QCPNP_RetrieveServiceConfig(PDEVICE_EXTENSION pDevExt)
{
   OBJECT_ATTRIBUTES oa;
   NTSTATUS          nts;
   HANDLE            hRegKey = NULL;
   UNICODE_STRING    ucValueName;
   ULONG             driverResident, selectiveSuspendInMili = 0, selectiveSuspendIdleTime = 0;
   ULONG             deviceAccess = SHARED_ACCESS;

   InitializeObjectAttributes(&oa, &gServicePath, 0, NULL, NULL);
   nts = ZwOpenKey(&hRegKey, KEY_READ, &oa);
   if (!NT_SUCCESS(nts))
   {
      _closeRegKey(hRegKey, "pnp-0a");
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_READ,
         QCSER_DBG_LEVEL_ERROR,
         ("<%s> PNP: reg open ERR\n", pDevExt->PortName)
      );
      return;
   }
   ucHdlCnt++;

   // driver residency
   RtlInitUnicodeString(&ucValueName, VEN_DRV_RESIDENT);
   nts = getRegDwValueEntryData
         (
            hRegKey,
            ucValueName.Buffer,
            &driverResident
         );

   if (nts == STATUS_SUCCESS)
   {
      if (driverResident != gVendorConfig.DriverResident)
      {
         QCSER_DbgPrint
         (
            QCSER_DBG_MASK_READ,
            QCSER_DBG_LEVEL_ERROR,
            ("<%s> PNP: DriverResident 0x%x\n", pDevExt->PortName, driverResident)
         );
         gVendorConfig.DriverResident = driverResident;
      }
   }

   // Get selective suspend idle time
   RtlInitUnicodeString(&ucValueName, VEN_DRV_SS_IDLE_T);
   nts = getRegDwValueEntryData
         (
            hRegKey,
            ucValueName.Buffer,
            &selectiveSuspendIdleTime
         );

   if (nts == STATUS_SUCCESS)
   {
      if (QCUTIL_IsHighSpeedDevice(pDevExt) == TRUE)
      {
         pDevExt->InServiceSelectiveSuspension = TRUE;
      }
      else
      {
         pDevExt->InServiceSelectiveSuspension = ((selectiveSuspendIdleTime >> 31) != 0);
      }
	  
	  selectiveSuspendInMili = selectiveSuspendIdleTime & 0x40000000;
	  
      selectiveSuspendIdleTime &= 0x00FFFFFF;

	  if (selectiveSuspendInMili == 0)
	  {
		  pDevExt->SelectiveSuspendInMiliSeconds = FALSE;
	      if ((selectiveSuspendIdleTime < QCUSB_SS_IDLE_MIN) &&
	          (selectiveSuspendIdleTime != 0))
	      {
	         selectiveSuspendIdleTime = QCUSB_SS_IDLE_MIN;
	      }
	      else if (selectiveSuspendIdleTime > QCUSB_SS_IDLE_MAX)
	      {
	         selectiveSuspendIdleTime = QCUSB_SS_IDLE_MAX;
	      }
	  }
	  else
	  {
		  pDevExt->SelectiveSuspendInMiliSeconds = TRUE;
		  if ((selectiveSuspendIdleTime < QCUSB_SS_MILI_IDLE_MIN) &&
			  (selectiveSuspendIdleTime != 0))
		  {
			 selectiveSuspendIdleTime = QCUSB_SS_MILI_IDLE_MIN;
		  }
		  else if (selectiveSuspendIdleTime > QCUSB_SS_MILI_IDLE_MAX)
		  {
			 selectiveSuspendIdleTime = QCUSB_SS_MILI_IDLE_MAX;
		  }
		  
	  }

      if (selectiveSuspendIdleTime != pDevExt->SelectiveSuspendIdleTime)
      {
         QCSER_DbgPrint
         (
            QCSER_DBG_MASK_READ,
            QCSER_DBG_LEVEL_ERROR,
            ("<%s> PNP: new selective suspend idle time=%us(%u)\n",
             pDevExt->PortName, selectiveSuspendIdleTime,
             pDevExt->InServiceSelectiveSuspension)
         );
         pDevExt->SelectiveSuspendIdleTime = selectiveSuspendIdleTime;
         QCPWR_SyncUpWaitWake(pDevExt);
         QCPWR_SetIdleTimer(pDevExt, 0, FALSE, 8);
      }
   }
#if 0
	  else
	  {
		  pDevExt->SelectiveSuspendInMiliSeconds = TRUE;
		  selectiveSuspendIdleTime = QCUSB_SS_MILI_IDLE_MIN;  // use default
		  pDevExt->InServiceSelectiveSuspension = TRUE;
   
		  if (pDevExt->SelectiveSuspendIdleTime != selectiveSuspendIdleTime)
		  {
			 QCSER_DbgPrint
			 (
				QCSER_DBG_MASK_READ,
				QCSER_DBG_LEVEL_ERROR,
				("<%s> INT: selective suspend changed to default\n", pDevExt->PortName)
			 );
			 pDevExt->SelectiveSuspendIdleTime = selectiveSuspendIdleTime;
			 QCPWR_SyncUpWaitWake(pDevExt);
			 QCPWR_SetIdleTimer(pDevExt, 0, FALSE, 10);
		  }   
	  }
#endif

   _closeRegKey(hRegKey, "pnp-1a");

}  // QCPNP_RetrieveServiceConfig

NTSTATUS QCPNP_StartDevice( IN  PDEVICE_OBJECT DeviceObject, IN UCHAR cookie )
{
   PDEVICE_EXTENSION pDevExt;
   NTSTATUS ntStatus;
   PURB pUrb;
   USHORT j;
   ULONG siz, i;
   PUSB_DEVICE_DESCRIPTOR deviceDesc = NULL;
   ANSI_STRING asUsbPath;
   UCHAR ConfigIndex;
   LARGE_INTEGER delayValue;

   pDevExt = DeviceObject -> DeviceExtension;
   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_DETAIL,
      ("<%s> StartDevice enter-%d: fdo 0x%p ext 0x%p\n", pDevExt->PortName, cookie,
        DeviceObject, pDevExt)
   );

   initDevState(DEVICE_STATE_PRESENT);

   // if (pDevExt->bFdoReused == TRUE)
   {
      if (pDevExt->pUsbDevDesc)
      {
         ExFreePool(pDevExt->pUsbDevDesc );
         pDevExt->pUsbDevDesc = NULL;
      }
      if (pDevExt->pUsbConfigDesc)
      {
         ExFreePool(pDevExt->pUsbConfigDesc );
         pDevExt->pUsbConfigDesc = NULL;
      }

      for (j = 0; j < MAX_INTERFACE; j++)
      {
         if (pDevExt->Interface[j] != NULL)
         {
            ExFreePool(pDevExt->Interface[j]);
            pDevExt->Interface[j] = NULL;
         }
      }
   }

   // First, get the device descriptor

   pUrb = ExAllocatePool
          (
             NonPagedPool,
             sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST)
          );
   if (pUrb)
   {
      deviceDesc = ExAllocatePool
                   (
                      NonPagedPool,
                      sizeof(USB_DEVICE_DESCRIPTOR)
                   );
      if (deviceDesc)
      {
         pDevExt -> pUsbDevDesc = deviceDesc;

         UsbBuildGetDescriptorRequest
         (
            pUrb,
            (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
            USB_DEVICE_DESCRIPTOR_TYPE,
            0,
            0,
            deviceDesc,
            NULL,
            sizeof(USB_DEVICE_DESCRIPTOR),
            NULL
         );

         ntStatus = QCUSB_CallUSBD( DeviceObject, pUrb );
         if(NT_SUCCESS(ntStatus))
         {

   #ifdef DEBUG_MSGS
            KdPrint (("Sample Device Descriptor:\n"));
            KdPrint (("-------------------------\n"));
            KdPrint (("bLength %d\n", deviceDesc->bLength));
            KdPrint (("bDescriptorType 0x%x\n", deviceDesc->bDescriptorType));
            KdPrint (("bcdUSB 0x%x\n", deviceDesc->bcdUSB));
            KdPrint (("bDeviceClass 0x%x\n", deviceDesc->bDeviceClass));
            KdPrint (("bDeviceSubClass 0x%x\n", deviceDesc->bDeviceSubClass));
            KdPrint (("bDeviceProtocol 0x%x\n", deviceDesc->bDeviceProtocol));
            KdPrint (("bMaxPacketSize0 0x%x\n", deviceDesc->bMaxPacketSize0));
            KdPrint (("idVendor 0x%x\n", deviceDesc->idVendor));
            KdPrint (("idProduct 0x%x\n", deviceDesc->idProduct));
            KdPrint (("bcdDevice 0x%x\n", deviceDesc->bcdDevice));
            KdPrint (("iManufacturer 0x%x\n", deviceDesc->iManufacturer));
            KdPrint (("iProduct 0x%x\n", deviceDesc->iProduct));
            KdPrint (("iSerialNumber 0x%x\n", deviceDesc->iSerialNumber));
            KdPrint (("bNumConfigurations 0x%x\n", deviceDesc->bNumConfigurations));
   #endif  //DEBUG_MSGS
            ExFreePool( pUrb );
            pUrb = NULL;
            pDevExt -> idVendor = deviceDesc  -> idVendor;
            pDevExt -> idProduct = deviceDesc -> idProduct;

            if (deviceDesc->bcdUSB >= QC_HSUSB_VERSION)
            {
               pDevExt->HighSpeedUsbOk |= QC_HSUSB_VERSION_OK;
            }

            if (deviceDesc->bcdUSB >= QC_SSUSB_VERSION)
            {
               pDevExt->HighSpeedUsbOk |= QC_SSUSB_VERSION_OK;
            }

            if (FALSE == QCPNP_ValidateDeviceDescriptor(pDevExt, pDevExt->pUsbDevDesc))
            {
               ntStatus = STATUS_UNSUCCESSFUL;
            }
         }
         else
         {
            if (ntStatus == STATUS_DEVICE_NOT_READY)
            {
               QCSER_DbgPrint
               (
                  QCSER_DBG_MASK_CONTROL,
                  QCSER_DBG_LEVEL_CRITICAL,
                  ("<%s> StartDevice: dev not ready\n", pDevExt->PortName)
               );
               // IoInvalidateDeviceState(pDevExt->PhysicalDeviceObject);
            }
            else
            {
               QCSER_DbgPrint
               (
                  QCSER_DBG_MASK_CONTROL,
                  QCSER_DBG_LEVEL_CRITICAL,
                  ("<%s> StartDevice: dev err-0 0x%x\n", pDevExt->PortName, ntStatus)
               );
            }
         }
      }  // if (deviceDesc)
      else
      {
         ntStatus = STATUS_NO_MEMORY;
         QCSER_DbgPrint
         (
            QCSER_DBG_MASK_CONTROL,
            QCSER_DBG_LEVEL_CRITICAL,
            ("<%s> StartDevice: STATUS_NO_MEMORY-0\n", pDevExt->PortName)
         );
      }
   }
   else
   {
      ntStatus = STATUS_NO_MEMORY;
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_CRITICAL,
         ("<%s> StartDevice: STATUS_NO_MEMORY-1\n", pDevExt->PortName)
      );
   }

   if (!NT_SUCCESS( ntStatus ))
   {
      if (pUrb != NULL)
      {
         ExFreePool(pUrb);
         pUrb = NULL;
      }
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_CRITICAL,
         ("<%s> StartDevice: failure 0x%x\n", pDevExt->PortName, ntStatus)
      );
      goto StartDevice_Return;         
   }

#if 0
   if (deviceDesc->bNumConfigurations > 1)
   {
      if (pUrb != NULL)
      {
         ExFreePool(pUrb);
         pUrb = NULL;
      }
      ntStatus = STATUS_UNSUCCESSFUL;
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_CRITICAL,
         ("<%s> StartDevice: bad numConfig %u\n", pDevExt->PortName, deviceDesc->bNumConfigurations)
      );
      goto StartDevice_Return;         
   }
#endif

   for (ConfigIndex = 0; ConfigIndex < deviceDesc->bNumConfigurations;
        ConfigIndex++)
   {
      ntStatus = QCPNP_ConfigureDevice( DeviceObject, ConfigIndex );
      if (NT_SUCCESS(ntStatus))
      {
         break;
      }
   }

   if (!NT_SUCCESS(ntStatus)) 
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_CRITICAL,
         ("<%s> StartDevice: failure-1 0x%x\n", pDevExt->PortName, ntStatus)
      );
      goto StartDevice_Return;
   }              

   pDevExt->bDeviceRemoved = FALSE;
   pDevExt->bDeviceSurpriseRemoved = FALSE;

   setDevState(DEVICE_STATE_USB_INITIALIZED);

   QCUSB_ByteStuffing(DeviceObject, TRUE);
   QCUSB_EnableBytePadding(DeviceObject);


   for ( i = 0; i < 10; i++ )
   {
      ntStatus = QCUSB_ResetInput(DeviceObject, QCUSB_RESET_PIPE_AND_ENDPOINT);
      if (ntStatus == STATUS_SUCCESS)
      {
         ntStatus = QCUSB_ResetOutput(DeviceObject, QCUSB_RESET_PIPE_AND_ENDPOINT);
      }
      if (NT_SUCCESS(ntStatus)) 
      {
         break;
      }
      else
      {
         LARGE_INTEGER timeoutValue;
         timeoutValue.QuadPart = -(4 * 1000 * 1000); // 0.4 sec
      
         QCSER_DbgPrint
         (
            QCSER_DBG_MASK_CONTROL,
            QCSER_DBG_LEVEL_ERROR,
            ("<%s> StartDevice: ResetInput/Output ResetPort %d err 0x%x\n", pDevExt->PortName, i, ntStatus)
         );
         
         // Sleep for 0.4sec
         KeDelayExecutionThread( KernelMode, FALSE, &timeoutValue );
      }
   }

   if (!NT_SUCCESS(ntStatus)) 
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_ERROR,
         ("<%s> StartDevice: ResetInput/Output err 0x%x\n", pDevExt->PortName, ntStatus)
      );
      goto StartDevice_Return;
   }

   // Configure power settings
   QCUTILS_PMGetRegEntryValues(pDevExt);
   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_DETAIL,
      ("<%s> PM_PowerManagementEnabled %u PM_WaitWakeEnabled %u Wake S%u\n", pDevExt->PortName, 
        pDevExt->PowerManagementEnabled, pDevExt->WaitWakeEnabled,
        (pDevExt->SystemWakeupLevel-1))
   );

   // Now, bring up interrupt listener
   QCINT_InitInterruptPipe(DeviceObject);

   // Register the device interface
   QCPNP_RegisterDeviceInterface(DeviceObject);
   
StartDevice_Return:

   if (!NT_SUCCESS(ntStatus))
   {
      clearDevState(DEVICE_STATE_PRESENT_AND_STARTED);
      clearDevState(DEVICE_STATE_USB_INITIALIZED);
   }
                  
   return ntStatus;
}  //_StartDevice

NTSTATUS QCPNP_ConfigureDevice
(
   IN PDEVICE_OBJECT DeviceObject,
   IN UCHAR ConfigIndex
)
{
   PDEVICE_EXTENSION pDevExt;
   NTSTATUS ntStatus;
   PURB pUrb;
   ULONG ulSize;
   PUSB_CONFIGURATION_DESCRIPTOR pConfigDesc;
   UCHAR *p, bmAttr;

   pDevExt = DeviceObject -> DeviceExtension;
   pUrb = ExAllocatePool
          (
             NonPagedPool,
             sizeof( struct _URB_CONTROL_DESCRIPTOR_REQUEST )
          );
   if (!pUrb)
   {
      return STATUS_NO_MEMORY;
   }
   // Set size of the data buffer.  Note we add padding to cover hardware
   // faults that may cause the device to go past the end of the data buffer

   pConfigDesc = ExAllocatePool
                 (
                    NonPagedPool,
                    sizeof(USB_CONFIGURATION_DESCRIPTOR)
                 );
   ASSERT(pConfigDesc != NULL);

   if (!pConfigDesc)
   {
      ExFreePool(pUrb);
      return STATUS_NO_MEMORY;
   }
   UsbBuildGetDescriptorRequest
   (
      pUrb,
      (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
      USB_CONFIGURATION_DESCRIPTOR_TYPE,
      ConfigIndex,                           // Index=0
      0x0,                                   // Language ID, not String
      pConfigDesc,
      NULL,                                  // MDL
      sizeof (USB_CONFIGURATION_DESCRIPTOR), // Get only the config desc */
      NULL                                   // Link
   );

   ntStatus = QCUSB_CallUSBD( DeviceObject, pUrb );
   if (!(NT_SUCCESS(ntStatus)))
   {
      ExFreePool(pConfigDesc);
      ExFreePool(pUrb);
      return ntStatus;
   }

   if (FALSE == QCPNP_ValidateConfigDescriptor(pDevExt, pConfigDesc))
   {
      ntStatus = STATUS_UNSUCCESSFUL;
      ExFreePool(pConfigDesc);
      ExFreePool(pUrb);
      return ntStatus;
   }

   // Determine how much data is in the entire configuration descriptor
   // and add extra room to protect against accidental overrun
   ulSize = pConfigDesc->wTotalLength;

   //  Free up the data buffer memory just used
   ExFreePool(pConfigDesc);
   pConfigDesc = NULL;
   pConfigDesc = ExAllocatePool(NonPagedPool, ulSize);

   // Now get the entire Configuration Descriptor
   if (!pConfigDesc)
   {
      ExFreePool(pUrb);
      return STATUS_NO_MEMORY;
   }
   UsbBuildGetDescriptorRequest
   (
      pUrb,
      (USHORT) sizeof (struct _URB_CONTROL_DESCRIPTOR_REQUEST),
      USB_CONFIGURATION_DESCRIPTOR_TYPE,
      ConfigIndex,               // Index=0
      0x0,                       // language ID
      pConfigDesc,
      NULL,
      ulSize,                    // Get all the descriptor data
      NULL                       // Link
   );

   ntStatus = QCUSB_CallUSBD( DeviceObject, pUrb );
   ExFreePool( pUrb );
   if(NT_SUCCESS(ntStatus))
   {
      pDevExt->pUsbConfigDesc = pConfigDesc;
   }
   else
   {
      if (pConfigDesc != NULL)
      {
         ExFreePool(pConfigDesc);
      }
      return ntStatus;
   }

   if (FALSE == QCPNP_ValidateConfigDescriptor(pDevExt, pConfigDesc))
   {
      ntStatus = STATUS_UNSUCCESSFUL;
      if (pConfigDesc != NULL)
      {
         ExFreePool(pConfigDesc);
      }
      pDevExt->pUsbConfigDesc = NULL;
      return ntStatus;
   }

   // Debug
   {
      int i;
      p = (UCHAR*)pConfigDesc;
      for (i=0; i < ulSize; i++)
      {
         // KdPrint((" (%d, 0x%x)-", i, p[i]));
      }
      // KdPrint(("\n"));
   }

   bmAttr = (UCHAR)pConfigDesc->bmAttributes;
   ntStatus = QCPNP_SelectInterfaces(DeviceObject, pConfigDesc);

   pDevExt->bRemoteWakeupEnabled = FALSE;
   if (NT_SUCCESS(ntStatus) && ((bmAttr & REMOTE_WAKEUP_MASK) != 0))
   {
      // Set remote wakeup feature, we don't care about the status
      QCUSB_ClearRemoteWakeup(DeviceObject);
      pDevExt->bRemoteWakeupEnabled = TRUE;
   }
   
   return ntStatus;
}  //QCPNP_ConfigureDevice

NTSTATUS QCPNP_SelectInterfaces
(
   IN PDEVICE_OBJECT pDevObj,
   IN PUSB_CONFIGURATION_DESCRIPTOR pConfigDesc
)
{
   PDEVICE_EXTENSION pDevExt;
   NTSTATUS ntStatus = STATUS_SUCCESS;
   PURB pUrb;
   ULONG siz, lTotalInterfaces, dwTempLen, x, i, j;
   UCHAR ucDataInterface = 0xff, ucNumPipes   = 0;
   UCHAR ucInterfaceNum  = 0,    ucAltSetting = 0;
   PUSB_INTERFACE_DESCRIPTOR pInterfaceDesc[MAX_INTERFACE], pIntdesc;
   PUSBD_INTERFACE_INFORMATION pInterfaceInfo[MAX_INTERFACE], pIntinfo;
   PUSBD_PIPE_INFORMATION pipeInformation;
   UCHAR ucPipeIndex;
   LONG lCommClassInterface = -1, lDataClassInterface = -1; // CDC only
   USBD_INTERFACE_LIST_ENTRY InterfaceList[MAX_INTERFACE];
   PVOID pStartPosition;
   PCDCC_FUNCTIONAL_DESCRIPTOR pFuncd;
   UCHAR *p = (UCHAR *)pConfigDesc;
   UCHAR *pDescEnd;
   GUID *featureGuid;
   BOOLEAN bObexModel = FALSE;
   /*****
      typedef struct {
          unsigned long  Data1;
          unsigned short Data2;
          unsigned short Data3;
          unsigned char  Data4[8];
      } GUID;
   *****/

   pDevExt = pDevObj -> DeviceExtension;

   pDevExt->BulkPipeInput    =
   pDevExt->BulkPipeOutput   =
   pDevExt->InterruptPipe    =
   pDevExt->InterruptPipeIdx = (UCHAR)-1;

   pDevExt->bBytePaddingFeature = FALSE;
   pDevExt->bEnableBytePadding  = FALSE;

   pDevExt->ControlInterface = pDevExt->DataInterface = 0;

   lTotalInterfaces = pConfigDesc->bNumInterfaces;

   #ifdef DEBUG_MSGS
   KdPrint (("selectinterface: Cfg->bLength=0x%x\n", (UCHAR)pConfigDesc->bLength));
   KdPrint (("selectinterface: Cfg->bDescType=0x%x\n", (UCHAR)pConfigDesc->bDescriptorType));
   KdPrint (("selectinterface: Cfg->wTotalLen=0x%x\n", (USHORT)pConfigDesc->wTotalLength));
   KdPrint (("selectinterface: Cfg->bNumInfs=0x%x\n", (UCHAR)lTotalInterfaces));
   KdPrint (("selectinterface: Cfg->bmAttr=0x%x\n", (UCHAR)pConfigDesc->bmAttributes));
   KdPrint (("selectinterface: Cfg->MaxPower=0x%x\n", (UCHAR)pConfigDesc->MaxPower));
   #endif

   // Init the interface pointers to NULL so they can be freed properly.
   for (x = 0; x < MAX_INTERFACE; x++)
   {
      InterfaceList[x].InterfaceDescriptor = NULL;
      InterfaceList[x].Interface           = NULL;
      pDevExt->Interface[x] = NULL;
   }
   // This should probably be more dynamic ...
   if (lTotalInterfaces > MAX_INTERFACE)
   {
      return STATUS_NO_MEMORY;
   }

   /*
    * Examine the config descriptor, which is followed by all the interface
    * descriptors such that each interface descriptor is followed by all the
    * endpoint descriptors for that interface, so that the size of the URB
    * needed to build the select configuration request can be determined.
    */

   // Parse the config descriptor to see whether the device is built with
   // byte stuffing feature (verify against a known GUID)
   pDescEnd = p + (USHORT)pConfigDesc->wTotalLength;
   while (p < pDescEnd)
   {
      // Mobile Direct Line Model Functional Descriptor
      if ((p[1] == USB_CDC_CS_INTERFACE) && (p[2] == USB_CDC_FD_MDLM))
      {
         // Now we see the WMC 1.0 functional descriptor which carries GUID for
         // special features. For byte-stuffing feature, we need to verify the GUID
         // (0x98b06a49, 0xb09e, 0x4896, 0x94, 0x46, 0xd9, 0x9a, 0x28, 0xca, 0x4e, 0x5d)
         featureGuid = (GUID *)(p+5);

         if ((featureGuid->Data1    == gQcFeatureGUID.Data1)    &&
             (featureGuid->Data2    == gQcFeatureGUID.Data2)    &&
             (featureGuid->Data3    == gQcFeatureGUID.Data3)    &&
             (featureGuid->Data4[0] == gQcFeatureGUID.Data4[0]) &&
             (featureGuid->Data4[1] == gQcFeatureGUID.Data4[1]) &&
             (featureGuid->Data4[2] == gQcFeatureGUID.Data4[2]) &&
             (featureGuid->Data4[3] == gQcFeatureGUID.Data4[3]) &&
             (featureGuid->Data4[4] == gQcFeatureGUID.Data4[4]) &&
             (featureGuid->Data4[5] == gQcFeatureGUID.Data4[5]) &&
             (featureGuid->Data4[6] == gQcFeatureGUID.Data4[6]) &&
             (featureGuid->Data4[7] == gQcFeatureGUID.Data4[7]))
         {
            pDevExt->bVendorFeature = TRUE;
            QCSER_DbgPrint
            (
               QCSER_DBG_MASK_READ,
               QCSER_DBG_LEVEL_CRITICAL,
               ("<%s>: VendorFeature ON\n", pDevExt->PortName)
            );
         }
      }
      // Mobile Direct Line Model Detail Functional Descriptor
      else if ((p[1] == USB_CDC_CS_INTERFACE) && (p[2] == USB_CDC_FD_MDLMD))
      {
         UCHAR ucLength = p[0];
         UCHAR bmControlCapabilities = p[4];
         UCHAR bmDataCapabilities = p[5];

         if (ucLength >= 6)
         {
            if ((bmControlCapabilities & 0x01) && (bmDataCapabilities & 0x01))
            {
               pDevExt->bByteStuffingFeature = TRUE;
               QCSER_DbgPrint
               (
                  QCSER_DBG_MASK_READ,
                  QCSER_DBG_LEVEL_CRITICAL,
                  ("<%s>: ByteStuffingFeature ON\n", pDevExt->PortName)
               );
            }
            else if ((bmControlCapabilities & 0x01) && (bmDataCapabilities & 0x10))
            {
               pDevExt->bBytePaddingFeature = TRUE;
               QCSER_DbgPrint
               (
                  QCSER_DBG_MASK_READ,
                  QCSER_DBG_LEVEL_CRITICAL,
                  ("<%s>: BytePaddingFeature ON\n", pDevExt->PortName)
               );
            }
            /*************************
            else if ((bmControlCapabilities & 0x01) && (bmDataCapabilities & 0x20))
            {
               pDevExt->DeviceFunction = QCUSB_DEV_FUNC_GPS;
               QCSER_DbgPrint
               (
                  QCSER_DBG_MASK_READ,
                  QCSER_DBG_LEVEL_CRITICAL,
                  ("<%s>: GPS ON\n", pDevExt->PortName)
               );
            }
            else
            {
               pDevExt->DeviceFunction = QCUSB_DEV_FUNC_UNDEF;
            }
            **************************/
         }
      }
      else if ((p[1] == USB_CDC_CS_INTERFACE) && (p[2] == USB_CDC_ACM_FD))
      {
         UCHAR bmCapabilities = p[3];

         if ((USB_CDC_ACM_SET_COMM_FEATURE_BIT_MASK & bmCapabilities) != 0)
         {
            pDevExt->SetCommFeatureSupported = TRUE;
         }
      }
      p += (UCHAR)*p;  // skip current descriptor
   }

   if (pDevExt->bVendorFeature == FALSE)
   {
      pDevExt->bByteStuffingFeature = FALSE;
      pDevExt->bBytePaddingFeature = FALSE;
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_READ,
         QCSER_DBG_LEVEL_CRITICAL,
         ("<%s> ByteStuffingFeature OFF\n", pDevExt->PortName)
      );
   }

   pStartPosition = (PVOID)((PCHAR)pConfigDesc + pConfigDesc->bLength);
   pIntdesc = pStartPosition;
   x = 0;
   while (pIntdesc != NULL)
   {
      /********
       typedef struct _USB_INTERFACE_DESCRIPTOR { 
                  UCHAR bLength ;
                  UCHAR bDescriptorType ;
                  UCHAR bInterfaceNumber ;
                  UCHAR bAlternateSetting ;
                  UCHAR bNumEndpoints ;
                  UCHAR bInterfaceClass ;
                  UCHAR bInterfaceSubClass ;
                  UCHAR bInterfaceProtocol ;
                  UCHAR iInterface ;
               } USB_INTERFACE_DESCRIPTOR, *PUSB_INTERFACE_DESCRIPTOR ;
      ***********/

      pIntdesc = USBD_ParseConfigurationDescriptorEx
                 (
                    pConfigDesc,
                    pStartPosition,
                    -1,
                    -1,
                    -1,
                    -1,
                    -1
                 );
      if (pIntdesc == NULL)
      {
         QCSER_DbgPrint
         (
            QCSER_DBG_MASK_READ,
            QCSER_DBG_LEVEL_CRITICAL,
            ("<%s> _SelectInterfaces: parse done, totalIF=%d\n",
              pDevExt->PortName, lTotalInterfaces)
         );
      }
      else
      {
         #ifdef DEBUG_MSGS
         KdPrint ((" --- SEARCH %d ---\n", x-1));
         KdPrint ((" - INF.bLength = 0x%x\n", (UCHAR)pIntdesc->bLength));
         KdPrint ((" - INF.bDescriptorType = 0x%x\n", (UCHAR)pIntdesc->bDescriptorType));
         KdPrint ((" - INF.bInterfaceNumber = 0x%x\n", (UCHAR)pIntdesc->bInterfaceNumber));
         KdPrint ((" - INF.bAlternateSetting = 0x%x\n", (UCHAR)pIntdesc->bAlternateSetting));
         KdPrint ((" - INF.bNumEndpoints = 0x%x\n", (UCHAR)pIntdesc->bNumEndpoints));
         KdPrint ((" - INF.bInterfaceClass = 0x%x\n", (UCHAR)pIntdesc->bInterfaceClass));
         KdPrint ((" - INF.bInterfaceSubClass = 0x%x\n", (UCHAR)pIntdesc->bInterfaceSubClass));
         KdPrint ((" - INF.bInterfaceProtocol = 0x%x\n", (UCHAR)pIntdesc->bInterfaceProtocol));
         #endif

         // to identify if it's an OBEX model
         if ((x == 0) &&
             (pIntdesc->bInterfaceClass == CDCC_COMMUNICATION_INTERFACE_CLASS) &&
             (pIntdesc->bInterfaceSubClass == CDCC_OBEX_INTERFACE_CLASS) &&
             (pIntdesc->bAlternateSetting == 0) &&
             (pIntdesc->bNumEndpoints == 0))
         {
            bObexModel = TRUE;
         }

         if ((bObexModel == TRUE) && (x > 0))
         {
            if ((pIntdesc->bAlternateSetting == 0) && 
                (pIntdesc->bInterfaceClass == CDCC_DATA_INTERFACE_CLASS) &&
                (pIntdesc->bInterfaceSubClass == 0) &&
                (pIntdesc->bInterfaceProtocol == 0))
            {
               QCSER_DbgPrint
               (
                  QCSER_DBG_MASK_CONTROL,
                  QCSER_DBG_LEVEL_DETAIL,
                  ("<%s> _SelectInterfaces: ignore OBEX reset IF\n", pDevExt->PortName)
               );
            }
            if ((pIntdesc->bAlternateSetting != 0) && 
                (pIntdesc->bInterfaceClass == CDCC_DATA_INTERFACE_CLASS) &&
                (pIntdesc->bInterfaceSubClass == 0) &&
                (pIntdesc->bInterfaceProtocol == 0))
            {
               QCSER_DbgPrint
               (
                  QCSER_DBG_MASK_CONTROL,
                  QCSER_DBG_LEVEL_DETAIL,
                  ("<%s> _SelectInterfaces: IF %u alt %u added to config[%u] \n",
                    pDevExt->PortName, pIntdesc->bInterfaceNumber, pIntdesc->bAlternateSetting, x)
               );
               InterfaceList[x++].InterfaceDescriptor = pIntdesc;
               pDevExt->HighSpeedUsbOk |= QC_HSUSB_ALT_SETTING_OK;
            }
         }
         else
         {
            if ((pIntdesc->bAlternateSetting != 0) && (pIntdesc->bNumEndpoints != 0))
            {
               // For HS-USB vendor-specific descriptors with alter_settings
               // Overwrite the previous one
               InterfaceList[--x].InterfaceDescriptor = pIntdesc;
               pDevExt->HighSpeedUsbOk |= QC_HSUSB_ALT_SETTING_OK;
            }
            else
            {
               InterfaceList[x++].InterfaceDescriptor = pIntdesc;
            }
            QCSER_DbgPrint
            (
               QCSER_DBG_MASK_CONTROL,
               QCSER_DBG_LEVEL_DETAIL,
               ("<%s> _SelectInterfaces: IF %u alt %u added to config[%u] \n",
                 pDevExt->PortName, pIntdesc->bInterfaceNumber, pIntdesc->bAlternateSetting, x)
            );
         }

         if (x >= MAX_INTERFACE)
         {
            QCSER_DbgPrint
            (
               QCSER_DBG_MASK_CONTROL,
               QCSER_DBG_LEVEL_CRITICAL,
               ("<%s> _SelectInterfaces: error -- too many interfaces\n", pDevExt->PortName)
            );
            return STATUS_UNSUCCESSFUL;
         }
         pStartPosition = (PVOID)((PCHAR)pIntdesc + pIntdesc->bLength);
      }
   } // while, for

   pUrb = USBD_CreateConfigurationRequestEx( pConfigDesc, InterfaceList );

   if ((pUrb == NULL)||(pUrb == (PURB)(-1)))
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_READ,
         QCSER_DBG_LEVEL_CRITICAL,
         ("<%s> _SelectInterfaces: err - USBD_CreateConfigurationRequestEx\n",
           pDevExt->PortName)
      );
      return STATUS_NO_MEMORY;
   }

   for (i = 0; i < MAX_INTERFACE; i++)
   {
      if (InterfaceList[i].Interface == NULL)
      {
         QCSER_DbgPrint2
         (
            QCSER_DBG_MASK_CONTROL,
            QCSER_DBG_LEVEL_DETAIL,
            ("<%s> [%d] - Interface is NULL\n", pDevExt->PortName, i)
         );
      }
      else
      {
         QCSER_DbgPrint2
         (
            QCSER_DBG_MASK_CONTROL,
            QCSER_DBG_LEVEL_DETAIL,
            ("<%s> [%d] - Interface is NOT NULL\n", pDevExt->PortName, i)
         );
      }
      if (InterfaceList[i].InterfaceDescriptor == NULL)
      {
         QCSER_DbgPrint2
         (
            QCSER_DBG_MASK_CONTROL,
            QCSER_DBG_LEVEL_DETAIL,
            ("<%s> [%d] - InterfaceDesc is NULL\n", pDevExt->PortName, i)
         );
         continue;
      }
      
      pIntdesc = InterfaceList[i].InterfaceDescriptor;
      pIntinfo = InterfaceList[i].Interface;
      if (pIntinfo == NULL)
      {
         QCSER_DbgPrint
         (
            QCSER_DBG_MASK_CONTROL,
            QCSER_DBG_LEVEL_CRITICAL,
            ("<%s> pIntinfo is NULL, FAILED!!!\n", pDevExt->PortName)
         );
         ExFreePool( pUrb );
         return STATUS_NO_MEMORY;
      }

      // Windows 2000: parent driver should set MaximumTransferSize to 4K
      // Windows XP/Server 2003: MaximumTransferSize is not used
      for (x = 0; x < pIntinfo->NumberOfPipes; x++)
      {
         pIntinfo->Pipes[x].MaximumTransferSize = QCUSB_MAX_PIPE_XFER_SIZE;
      }
      ucNumPipes += pIntinfo -> NumberOfPipes;
      pIntinfo -> Length = GET_USBD_INTERFACE_SIZE( pIntdesc -> bNumEndpoints );
      pIntinfo -> InterfaceNumber = pIntdesc -> bInterfaceNumber;
      pIntinfo -> AlternateSetting = pIntdesc -> bAlternateSetting;

   } // for (interface list[i++])

   if (ucNumPipes > MAX_IO_QUEUES)
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_CRITICAL,
         ("<%s> _SelectInterfaces: too many pipes %d\n", pDevExt->PortName, ucNumPipes)
      );
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   #ifdef DEBUG_MSGS
   KdPrint (("_selectinterface (pipes %d): 9999 \n", ucNumPipes));
   KdPrint
   (
      ("Conf URB  Length: %d, Function: %x\n",
          pUrb -> UrbSelectConfiguration.Hdr.Length,
          pUrb -> UrbSelectConfiguration.Hdr.Function)
   );

   KdPrint
   (
      ("ConfigurationDesc: %x, ConfigurationHandle: %x\n",
         pUrb -> UrbSelectConfiguration.ConfigurationDescriptor,
         pUrb -> UrbSelectConfiguration.ConfigurationHandle)
   );
   #endif // DEBUG_MSGS

   ntStatus = QCUSB_CallUSBD( pDevObj, pUrb );

   #ifdef DEBUG_MSGS
   if (NT_SUCCESS(ntStatus))
   {
      KdPrint((" CONFIG NTSTATUS = SUCCESS!!"));
   }
   else
   {
      KdPrint((" CONFIG Failed NTSTATUS = 0x%x...\n", ntStatus));
   }

   if (USBD_SUCCESS(pUrb->UrbSelectConfiguration.Hdr.Status))
   {
      KdPrint((" CONFIG URB: NTSTATUS_SUCCESS\n\n"));
   }
   else
   {
      KdPrint((" CONFIG URB Failed NTSTATUS = 0x%x...\n\n",
                pUrb->UrbSelectConfiguration.Hdr.Status));
   }
   #endif // DEBUG_MSGS

   if ((NT_SUCCESS(ntStatus)) &&
       (USBD_SUCCESS(pUrb->UrbSelectConfiguration.Hdr.Status)))
   {
      pDevExt->ConfigurationHandle = 
         pUrb->UrbSelectConfiguration.ConfigurationHandle;

      for (x = 0; x < lTotalInterfaces; x++)
      {
         // The length field includes all the pipe structures that follow the
         // the interface descriptor.

         pIntinfo = InterfaceList[x].Interface;
         dwTempLen = pIntinfo -> Length;
         pDevExt->Interface[pIntinfo->InterfaceNumber] = ExAllocatePool
                                                         (
                                                            NonPagedPool, dwTempLen
                                                         );
         if (pDevExt->Interface[pIntinfo->InterfaceNumber])
         {
            // Save the interface descriptors and pipe info structures in
            // the device extension.
            RtlCopyMemory
            (
               pDevExt->Interface[pIntinfo->InterfaceNumber],
               pIntinfo, dwTempLen
            );

            // Note: the following code will not work if there is more than
            // one interface. see the synopsis at the beginning of this
            // module.
            for (j = 0; j < pIntinfo -> NumberOfPipes; j++)
            {
               pipeInformation = &pIntinfo->Pipes[j];
               ucPipeIndex = j;
               if (pipeInformation->PipeType == UsbdPipeTypeBulk)
               {
                  if (((pipeInformation->EndpointAddress)&0x80) == 0) //OUT?
                  {
                     pDevExt->lWriteBufferUnit = pDevExt->WriteUnitSize;
                        // (LONG) pipeInformation -> MaximumPacketSize;
                     pDevExt->wMaxPktSize =
                        pipeInformation->MaximumPacketSize;
                     if (pDevExt->MinInPktSize < pDevExt->wMaxPktSize)
                     {
                        pDevExt->MinInPktSize = pDevExt->wMaxPktSize;
                     }
                     if (pDevExt->BulkPipeOutput == (UCHAR) -1) //take the 1st
                     {
                        pDevExt->DataInterface = pIntinfo->InterfaceNumber;
                        pDevExt->BulkPipeOutput = ucPipeIndex; //save index
                     }
                  }
                  else // IN?
                  {
                     if (pDevExt -> BulkPipeInput == (UCHAR) -1)
                     {
                        pDevExt -> BulkPipeInput = ucPipeIndex;
                     }
                  }
               }
               else if (pipeInformation->PipeType == UsbdPipeTypeInterrupt)
               {
                  if (pDevExt -> InterruptPipe == (UCHAR)-1)
                  {
                     pDevExt->usCommClassInterface = (USHORT)pIntinfo->InterfaceNumber;
                     // Control interface, if not specifically defined, should always
                     // be 0, so we do not assign a value here for now.
                     // pDevExt->ControlInterface = pIntinfo->InterfaceNumber;
                     pDevExt->InterruptPipe = pDevExt->InterruptPipeIdx = ucPipeIndex;
                  }
               } //if

               #ifdef DEBUG_MSGS
               // Dump the pipe info
               KdPrint(("---------\n"));
               KdPrint(("PipeType 0x%x\n", pipeInformation->PipeType));
               KdPrint(("EndpointAddress 0x%x\n", pipeInformation->EndpointAddress));
               KdPrint(("MaxPacketSize 0x%x\n", pipeInformation->MaximumPacketSize));
               KdPrint(("Interval 0x%x\n", pipeInformation->Interval));
               KdPrint(("Handle 0x%p\n", pipeInformation->PipeHandle));
               #endif // DEBUG_MSGS
            } //for
         } //if
         else
         {
            QCSER_DbgPrint
            (
               QCSER_DBG_MASK_CONTROL,
               QCSER_DBG_LEVEL_CRITICAL,
               ("<%s> _SelectInterfaces: no mem - if\n", pDevExt->PortName)
            );
            ntStatus = STATUS_NO_MEMORY;
            break;
         }
      }  // for loop
      ExFreePool( pUrb );
   } // if

   if (pDevExt->wMaxPktSize == QC_HSUSB_BULK_MAX_PKT_SZ)
   {
      pDevExt->HighSpeedUsbOk |= QC_HSUSB_BULK_MAX_PKT_OK;
   }

   //  Test interfaces for valid modem type, LEGACY or CDC
   // Make sure all the required pipes are present
   if (NT_SUCCESS(ntStatus))
   {
      if (pDevExt->BulkPipeInput  != (UCHAR)-1 &&
          pDevExt->BulkPipeOutput != (UCHAR)-1)
      {
          if (pDevExt->InterruptPipe  != (UCHAR)-1)
          {
             pDevExt->ucDeviceType = DEVICETYPE_CDC;
             DbgPrint("\n   ============ Loaded ==========\n");
             DbgPrint("   | Device Type: CDC-TYPE      |\n");
             DbgPrint("   |   Version: %-10s      |\n", gVendorConfig.DriverVersion);
             DbgPrint("   |   Device:  %-10s      |\n", pDevExt->PortName);
             DbgPrint("   |   IF: CT%02d-CC%02d-DA%02d       |\n", pDevExt->ControlInterface,
                       pDevExt->usCommClassInterface, pDevExt->DataInterface);
             DbgPrint("   |   EP(0x%x, 0x%x, 0x%x) HS 0x%x   |\n",
                         pDevExt->Interface[pDevExt->DataInterface]
                            ->Pipes[pDevExt->BulkPipeInput].EndpointAddress,
                         pDevExt->Interface[pDevExt->DataInterface]
                            ->Pipes[pDevExt->BulkPipeOutput].EndpointAddress,
                         pDevExt->Interface[pDevExt->usCommClassInterface]
                            ->Pipes[pDevExt->InterruptPipe].EndpointAddress,
                         pDevExt->HighSpeedUsbOk);
             DbgPrint("Driver Version %s\n", "2.1.2.2");
             DbgPrint("   |============================|\n");
          }
          else
          {
             if ((pDevExt->lWriteBufferUnit == USB_WRITE_UNIT_SIZE) &&
                 (USB_WRITE_UNIT_SIZE < 2048L))
             {
                pDevExt->lWriteBufferUnit = 2048L;
             }
             pDevExt->ucDeviceType = DEVICETYPE_SERIAL;
             DbgPrint("\n   ============== Loaded ===========\n");
             DbgPrint("   | DeviceType: SERIAL            |\n");
             DbgPrint("   |   Version: %-10s         |\n", gVendorConfig.DriverVersion);
             DbgPrint("   |   Device:  %-10s         |\n", pDevExt->PortName);
             DbgPrint("   |   IF: CT%02d-CC%02d-DA%02d          |\n", pDevExt->ControlInterface,
                       pDevExt->usCommClassInterface, pDevExt->DataInterface);
             DbgPrint("   |   EP(0x%x, 0x%x) HS 0x%x   |\n",
                         pDevExt->Interface[pDevExt->DataInterface]
                            ->Pipes[pDevExt->BulkPipeInput].EndpointAddress,
                         pDevExt->Interface[pDevExt->DataInterface]
                            ->Pipes[pDevExt->BulkPipeOutput].EndpointAddress,
                         pDevExt->HighSpeedUsbOk);
             DbgPrint("Driver Version %s\n", "2.1.2.2");
             DbgPrint("   |===============================|\n");
          }
          QCSER_DbgPrint
          (
             QCSER_DBG_MASK_CONTROL,
             QCSER_DBG_LEVEL_FORCE,
             ("<%s> I/O units: %ld/%ld/%ld\n\n", pDevExt->PortName,
               pDevExt->MaxPipeXferSize, pDevExt->MinInPktSize, pDevExt->lWriteBufferUnit)
          );

          QCSER_DbgPrint
          (
             QCSER_DBG_MASK_CONTROL,
             QCSER_DBG_LEVEL_FORCE,
             ("<%s> Aggregation time: %ldus Max Size: %u Bytes\n\n", pDevExt->PortName,
               pDevExt->AggregationPeriod, pDevExt->WriteAggBuffer.MaxSize)
          );
		  
          pDevExt->InServiceSelectiveSuspension = QCUTIL_IsHighSpeedDevice(pDevExt);

          if (pDevExt->wMaxPktSize == 0)
          {
             ntStatus = STATUS_UNSUCCESSFUL;
             QCSER_DbgPrint
             (
                QCSER_DBG_MASK_CONTROL,
                QCSER_DBG_LEVEL_CRITICAL,
                ("<%s> _SelectInterfaces: error - wMaxPktSize=0\n", pDevExt->PortName)
             );
          }
      }
      else if ((pDevExt->BulkPipeInput  == (UCHAR)-1) &&
               (pDevExt->BulkPipeOutput == (UCHAR)-1) &&
               (pDevExt->InterruptPipe  == (UCHAR)-1))
      {
         pDevExt->ucDeviceType = DEVICETYPE_CTRL;
         DbgPrint("\n   ============== Loaded ===========\n");
         DbgPrint("   | DeviceType: CONTROL           |\n");
         DbgPrint("   |   Version: %-10s         |\n", gVendorConfig.DriverVersion);
         DbgPrint("   |   Device:  %-10s         |\n", pDevExt->PortName);
         DbgPrint("   |   IF: CT%02d-CC%02d-DA%02d          |\n", pDevExt->ControlInterface,
                   pDevExt->usCommClassInterface, pDevExt->DataInterface);
         DbgPrint("   |   HS 0x%x       |\n", pDevExt->HighSpeedUsbOk);
         DbgPrint("Driver Version 2.1.2.2\n");
         DbgPrint("   |===============================|\n\n");
      }
      else
      {
         ntStatus = STATUS_INSUFFICIENT_RESOURCES;
         DbgPrint("\n   ==============================\n");
         DbgPrint("   | Device Type: NONE          |\n");
         DbgPrint("   |============================|\n\n");
         pDevExt->ucDeviceType = DEVICETYPE_INVALID;
      }
   } // if (NT_SUCCESS)

   if (!NT_SUCCESS(ntStatus))
   {
      for (i = 0; i < MAX_INTERFACE; i++)
      {
         if (pDevExt->Interface[i] != NULL)
         {
            ExFreePool(pDevExt->Interface[i]);
            pDevExt->Interface[i] = NULL;
         }
      }
   } // if (!NT_SUCCESS)

   return ntStatus;
}  // QCPNP_SelectInterfaces

NTSTATUS QCPNP_HandleRemoveDevice
(
   IN PDEVICE_OBJECT DeviceObject,
   IN PDEVICE_OBJECT CalledDO,
   IN PIRP Irp,
   IN BOOLEAN RemoveFdoOnly
)
{
   KEVENT ePNPEvent;
   PDEVICE_EXTENSION pDevExt;
   PFDO_DEVICE_EXTENSION pFdoExt;
   NTSTATUS ntStatus = STATUS_SUCCESS;
   PDEVICE_OBJECT myFDO;
   #ifdef QCSER_TARGET_XP
   KLOCK_QUEUE_HANDLE levelOrHandle;
   #else
   KIRQL levelOrHandle;
   #endif


   pDevExt = DeviceObject->DeviceExtension;

   if (RemoveFdoOnly == FALSE)
   {
      QCDEV_DeregisterDeviceInterface(pDevExt);
   }

   myFDO = CalledDO;
   
   if (myFDO == NULL)
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_CRITICAL,
         ("<%s> QCPNP_HandleRemoveDevice: NULL FDO - 0x%p\n", pDevExt->PortName, DeviceObject)
      );
      return STATUS_UNSUCCESSFUL;
   }
   pFdoExt = myFDO->DeviceExtension;

   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_DETAIL,
      ("<%s> QCPNP_HandleRemoveDevice: FDO 0x%p::0x%p\n", pDevExt->PortName, myFDO, DeviceObject)
   );

   KeInitializeEvent(&ePNPEvent, SynchronizationEvent, FALSE);

   // Send the IRP down the stack FIRST, to finish the removal

   IoCopyCurrentIrpStackLocationToNext(Irp);

   IoSetCompletionRoutine
   (
      Irp,
      QCMAIN_IrpCompletionSetEvent,
      &ePNPEvent,
      TRUE,TRUE,TRUE
   );

   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_DETAIL,
      ("<%s> QCPNP_HandleRemoveDevice: IoCallDriver 0x%p\n", pDevExt->PortName, pFdoExt->StackDeviceObject)
   );
   KeClearEvent(&ePNPEvent);
   ntStatus = IoCallDriver
              (
                 pFdoExt->StackDeviceObject,
                 Irp
              );
   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_DETAIL,
      ("<%s> QCPNP_HandleRemoveDevice: IoCallDriver st 0x%x \n", pDevExt->PortName, ntStatus)
   );
   ntStatus = KeWaitForSingleObject
              (
                 &ePNPEvent, Executive, KernelMode, FALSE, NULL
              );
   KeClearEvent(&ePNPEvent);

   IoDetachDevice(pFdoExt->StackDeviceObject);

   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_DETAIL,
      ("<%s> QCPNP_HandleRemoveDevice: Deleting FDO...\n", pDevExt->PortName)
   );
   IoDeleteDevice(myFDO);

   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_INFO,
      ("<%s> QCPNP_HandleRemoveDevice (%ld,%ld,%ld,%ld)\n", pDevExt->PortName,
        pDevExt->Sts.lRmlCount[0], pDevExt->Sts.lRmlCount[1], pDevExt->Sts.lRmlCount[2], pDevExt->Sts.lRmlCount[3])
   );

   Irp->IoStatus.Status = STATUS_SUCCESS;

   return ntStatus;
} // QCPNP_HandleRemoveDevice

NTSTATUS QCPNP_StopDevice( IN  PDEVICE_OBJECT DeviceObject )
{
   PDEVICE_EXTENSION deviceExtension;
   NTSTATUS ntStatus = STATUS_SUCCESS;
   PURB pUrb;
   ULONG siz;

   deviceExtension = DeviceObject -> DeviceExtension;

   // send the select configuration urb with
   // a NULL pointer for the configuration handle
   // this closes the configuration and puts the
   // device in the 'unconfigured' state.

   siz = sizeof(struct _URB_SELECT_CONFIGURATION);
   pUrb = ExAllocatePool(NonPagedPool, siz);
   if (pUrb)
   {
      UsbBuildSelectConfigurationRequest(pUrb, (USHORT) siz, NULL);
      ntStatus = QCUSB_CallUSBD(DeviceObject, pUrb);
      ExFreePool(pUrb);
   }
   else
   {
      ntStatus = STATUS_NO_MEMORY;
   }

   return ntStatus;
}  // QCPNP_StopDdevice

BOOLEAN QCPNP_CreateSymbolicLink(PDEVICE_OBJECT DeviceObject)
{
   PDEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;
   NTSTATUS ntStatus;
   
   ntStatus = IoCreateUnprotectedSymbolicLink
              (
                 &pDevExt->ucsUnprotectedLink,
                 &pDevExt->ucsDeviceMapEntry
              );

   // if the link already exist in system (STATUS_OBJECT_NAME_COLLISION)
   // we'd like to ignore the error and update the registry
   if (!NT_SUCCESS(ntStatus) && (ntStatus != STATUS_OBJECT_NAME_COLLISION))
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_ERROR,
         ("<%s> QCPNP_CreateSymbolicLink: ERR sym lnk - 0x%x\n", pDevExt->PortName, ntStatus)
      );
      return FALSE;
   }

   // put our entry in the hardware device map
   ntStatus = RtlWriteRegistryValue
              (
                 RTL_REGISTRY_DEVICEMAP,
                 L"SERIALCOMM",
                 pDevExt->ucsDeviceMapEntry.Buffer,          // "\Device\QCOMSERn(nn)"
                 REG_SZ,
                 pDevExt->ucsPortName.Buffer,                // "COMn(n)"
                 pDevExt->ucsPortName.Length + sizeof(WCHAR)
              );
   if (!NT_SUCCESS(ntStatus))
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_CRITICAL,
         ("<%s> QCPNP_CreateSymbolicLink: ERR DEVICEMAP - 0x%x\n", pDevExt->PortName, ntStatus)
      );
      return FALSE;
   }

   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_DETAIL,
      ("<%s> CreateSymbolicLink: TRUE\n", pDevExt->PortName)
   );

   return TRUE;
}  // QCPNP_CreateSymbolicLink

DEVICE_TYPE QCPNP_GetDeviceType(PDEVICE_OBJECT PortDO)
{
   #define DEV_CLASS_UNKNOWN 0
   #define DEV_CLASS_PORTS   1
   #define DEV_CLASS_MODEM   2

   PDEVICE_EXTENSION pDevExt = PortDO->DeviceExtension;
   HANDLE            hRegKey = NULL;
   NTSTATUS          ntStatus;
   CHAR              className[128];
   ULONG             bufLen = 128, resultLen = 0;
   PCHAR             classPorts = "P o r t s   ";  // len = 12
   PCHAR             classModem = "M o d e m   ";  // len = 12
   UCHAR             classType = DEV_CLASS_UNKNOWN;
   DEVICE_TYPE       deviceType = FILE_DEVICE_UNKNOWN;

   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_TRACE,
      ("<%s> -->GetDeviceType PDO 0x%p\n", pDevExt->PortName, pDevExt->PhysicalDeviceObject)
   );

   className[0] = 0;

   ntStatus = IoGetDeviceProperty
              (
                 pDevExt->PhysicalDeviceObject,
                 DevicePropertyClassName,
                 bufLen,
                 (PVOID)className,
                 &resultLen
              );

   if (ntStatus == STATUS_SUCCESS)
   {
      int i;

      // extract class name into an ANSI string
      for (i = 0; i < resultLen; i++)
      {
         if (i >= 30)
         {
            i++;
            break;
         }
         if (className[i] == 0)
         {
            className[i] = ' ';
         }
         QCSER_DbgPrint2
         (
            QCSER_DBG_MASK_CONTROL,
            QCSER_DBG_LEVEL_FORCE,
            ("<%s> GetDeviceType: className[%u]=%c\n", pDevExt->PortName, i, className[i])
         );
      }
      className[i] = 0;

      if (RtlCompareMemory(className, classPorts, 12) == 12)
      {
         deviceType = FILE_DEVICE_SERIAL_PORT;
      }
      else if (RtlCompareMemory(className, classModem, 12) == 12)
      {
         deviceType = FILE_DEVICE_MODEM;
      }
      else
      {
         deviceType = FILE_DEVICE_UNKNOWN;
      }
   }
   else
   {
      deviceType = FILE_DEVICE_UNKNOWN;
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_ERROR,
         ("<%s> GetDeviceType: property query err\n", pDevExt->PortName)
      );
   }

   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_TRACE,
      ("<%s> <--GetDeviceType: class=%s[%u] devType 0x%x ST 0x%x\n",
        pDevExt->PortName, className, classType, deviceType, ntStatus)
   );

   return deviceType;
}  // QCPNP_GetDeviceType

NTSTATUS QCPNP_RegisterDeviceInterface(PDEVICE_OBJECT DeviceObject)
{
   #define DEV_CLASS_UNKNOWN 0
   #define DEV_CLASS_PORTS   1
   #define DEV_CLASS_MODEM   2

   PDEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;
   NTSTATUS          ntStatus = STATUS_UNSUCCESSFUL;

   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_TRACE,
      ("<%s> -->RegisterDeviceInterface\n", pDevExt->PortName)
   );

   switch (pDevExt->FdoDeviceType)
   {
      case FILE_DEVICE_SERIAL_PORT:
      {
         ntStatus = IoRegisterDeviceInterface
                    (
                       pDevExt->PhysicalDeviceObject,
                       &GUID_DEVINTERFACE_PORTS,
                       NULL, 
                       &pDevExt->ucsIfaceSymbolicLinkName
                    );
         break;
      }
      case FILE_DEVICE_MODEM:
      {
         ntStatus = IoRegisterDeviceInterface
                    (
                       pDevExt->PhysicalDeviceObject,
                       &GUID_DEVINTERFACE_MODEM,
                       NULL, 
                       &pDevExt->ucsIfaceSymbolicLinkName
                    );
         break;
      }
   }

   if (NT_SUCCESS(ntStatus) && 
       ((pDevExt->FdoDeviceType == FILE_DEVICE_SERIAL_PORT) ||
        (pDevExt->FdoDeviceType == FILE_DEVICE_MODEM)))
   {
      ntStatus = IoSetDeviceInterfaceState
                 (
                    &pDevExt->ucsIfaceSymbolicLinkName, TRUE
                 );
      if (!NT_SUCCESS(ntStatus))
      {
         QCSER_DbgPrint
         (
            QCSER_DBG_MASK_CONTROL,
            QCSER_DBG_LEVEL_ERROR,
            ("<%s> IoSetDeviceInterfaceState: failure 0x%x\n", pDevExt->PortName, ntStatus)
         );
      }
      else
      {
         #ifdef QCSER_POKE_MEMORY
         int i;
         PCHAR symName = (PCHAR)(pDevExt->ucsIfaceSymbolicLinkName.Buffer);

         for (i = 0; i < pDevExt->ucsIfaceSymbolicLinkName.Length*2; i++)
         {
            QCSER_DbgPrint
            (
               QCSER_DBG_MASK_CONTROL,
               QCSER_DBG_LEVEL_FORCE,
               ("<%s> IFSymName[%u]=%c\n", pDevExt->PortName, i, symName[i])
            );
         }
         #endif  // QCSER_POKE_MEMORY

         QCSER_DbgPrint
         (
            QCSER_DBG_MASK_CONTROL,
            QCSER_DBG_LEVEL_TRACE,
            ("<%s> IoSetDeviceInterfaceState: enabled\n", pDevExt->PortName)
         );
      }
   }
   else
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_ERROR,
         ("<%s> RegisterDeviceInterface: failure\n", pDevExt->PortName)
      );
   }

   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_TRACE,
      ("<%s> <--RegisterDeviceInterface: ST 0x%x\n", pDevExt->PortName, ntStatus)
   );

   return ntStatus;
}  // QCPNP_RegisterDeviceInterface

BOOLEAN QCPNP_ValidateConfigDescriptor
(
   PDEVICE_EXTENSION pDevExt,
   PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc
)
{
   if ((ConfigDesc->bLength ==0) ||
       (ConfigDesc->bLength > 9) ||
       (ConfigDesc->wTotalLength == 0) ||
       (ConfigDesc->wTotalLength < 9))
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_ERROR,
         ("<%s> _ValidateConfigDescriptor: bad length: %uB, %uB\n",
           pDevExt->PortName, ConfigDesc->bLength, ConfigDesc->wTotalLength
         )
      );
      return FALSE;
   }

   if (ConfigDesc->bNumInterfaces >= MAX_INTERFACE)
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_ERROR,
         ("<%s> _ValidateConfigDescriptor: bad bNumInterfaces 0x%x\n",
           pDevExt->PortName, ConfigDesc->bNumInterfaces)
      );
      return FALSE;
   }

   if (ConfigDesc->bDescriptorType != 0x02)
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_ERROR,
         ("<%s> _ValidateConfigDescriptor: bad bDescriptorType 0x%x\n",
           pDevExt->PortName, ConfigDesc->bDescriptorType)
      );
      return FALSE;
   }

   if (ConfigDesc->bConfigurationValue != 1)
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_ERROR,
         ("<%s> _ValidateConfigDescriptor: bad bConfigurationValue 0x%x\n",
           pDevExt->PortName, ConfigDesc->bConfigurationValue)
      );
      return FALSE;
   }

   return TRUE;

}  // QCPNP_ValidateConfigDescriptor

BOOLEAN QCPNP_ValidateDeviceDescriptor
(
   PDEVICE_EXTENSION      pDevExt,
   PUSB_DEVICE_DESCRIPTOR DevDesc
)
{
   if (DevDesc->bLength == 0)
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_ERROR,
         ("<%s> _ValidateDeviceDescriptor: 0 bLength\n", pDevExt->PortName)
      );
      return FALSE;
   }

   if (DevDesc->bDescriptorType != 0x01)
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_ERROR,
         ("<%s> _ValidateDeviceDescriptor: bad bDescriptorType 0x%x\n",
           pDevExt->PortName, DevDesc->bDescriptorType)
      );
      return FALSE;
   }

   if ((pDevExt->HighSpeedUsbOk & QC_SSUSB_VERSION_OK) == 0)  // if not SS USB
   {
      if ((DevDesc->bMaxPacketSize0 != 0x08) &&
          (DevDesc->bMaxPacketSize0 != 0x10) &&
          (DevDesc->bMaxPacketSize0 != 0x20) &&
          (DevDesc->bMaxPacketSize0 != 0x40))
      {
         QCSER_DbgPrint
         (
            QCSER_DBG_MASK_CONTROL,
            QCSER_DBG_LEVEL_ERROR,
            ("<%s> _ValidateDeviceDescriptor: bad bMaxPacketSize0 0x%x\n",
              pDevExt->PortName, DevDesc->bMaxPacketSize0)
         );
         return FALSE;
      }
   }
   else
   {
      if (DevDesc->bMaxPacketSize0 != 0x09)
      {
         QCSER_DbgPrint
         (
            QCSER_DBG_MASK_CONTROL,
            QCSER_DBG_LEVEL_ERROR,
            ("<%s> _ValidateDeviceDescriptor: bad SS bMaxPacketSize0 0x%x\n",
              pDevExt->PortName, DevDesc->bMaxPacketSize0)
         );
         return FALSE;
      }
   }

#if 0
   if (DevDesc->bNumConfigurations != 0x01)
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_ERROR,
         ("<%s> _ValidateDeviceDescriptor: bad bNumConfigurations 0x%x\n",
           pDevExt->PortName, DevDesc->bNumConfigurations)
      );
      return FALSE;
   }
#endif

   return TRUE;

}  // QCPNP_ValidateDeviceDescriptor

NTSTATUS QCPNP_ReportDeviceName(PDEVICE_EXTENSION pDevExt)
{
   NTSTATUS          ntStatus;
   CHAR              devName[QCDEV_NAME_LEN_MAX], tmpName[QCDEV_NAME_LEN_MAX];
   ULONG             bufLen = QCDEV_NAME_LEN_MAX, resultLen = 0;
   PCHAR             matchText = "D i a g n o s t i c s ";

   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_TRACE,
      ("<%s> -->ReportDeviceName PDO 0x%p\n", pDevExt->PortName, pDevExt->PhysicalDeviceObject)
   );

   RtlZeroMemory(devName, QCDEV_NAME_LEN_MAX);
   RtlZeroMemory(tmpName, QCDEV_NAME_LEN_MAX);

   ntStatus = IoGetDeviceProperty
              (
                 pDevExt->PhysicalDeviceObject,
                 DevicePropertyFriendlyName,
                 bufLen,
                 (PVOID)devName,
                 &resultLen
              );

   if (ntStatus != STATUS_SUCCESS)
   {
      ntStatus = IoGetDeviceProperty
                 (
                    pDevExt->PhysicalDeviceObject,
                    DevicePropertyDeviceDescription,
                    bufLen,
                    (PVOID)devName,
                    &resultLen
                 );
   }

   if (ntStatus != STATUS_SUCCESS)
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_TRACE,
         ("<%s> <--ReportDeviceName PDO 0x%p failure 0x%X\n", pDevExt->PortName, pDevExt->PhysicalDeviceObject, ntStatus)
      );
      return ntStatus;
   }
   else
   {
      PTSTR matchPtr;
      int i;

      RtlCopyMemory(tmpName, devName, resultLen);

      for (i = 0; i < resultLen; i++)
      {
         if (tmpName[i] == 0)
         {
            tmpName[i] = ' ';
         }
      }
      tmpName[i] = 0;

      matchPtr = strstr(tmpName, matchText);

      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_TRACE,
         ("<%s> ReportDeviceName <%s> match 0x%p\n", pDevExt->PortName, tmpName, matchPtr)
      );

      if (matchPtr != NULL)
      {
         ntStatus = QCPNP_RegisterDevName
                    (
                       pDevExt,
                       IOCTL_QCUSB_REPORT_DEV_NAME,
                       devName,
                       resultLen
                    );
      }
   }

   return ntStatus;

}  // QCPNP_ReportDeviceName

NTSTATUS QCPNP_RegisterDevName
(
   PDEVICE_EXTENSION pDevExt,
   ULONG       IoControlCode,
   PVOID       Buffer,
   ULONG       BufferLength
)
{

   PIRP pIrp = NULL;
   PIO_STACK_LOCATION nextstack;
   NTSTATUS Status;
   KEVENT Event;
   PPEER_DEV_INFO_HDR pDevInfoHdr;
   PCHAR pLocation;
   ULONG totalLength;
   
   KeInitializeEvent(&Event, SynchronizationEvent, FALSE);
   
   pIrp = IoAllocateIrp((CCHAR)(pDevExt->StackDeviceObject->StackSize+2), FALSE);
   if( pIrp == NULL )
   {
       QCSER_DbgPrint
       (
          QCSER_DBG_MASK_CONTROL,
          QCSER_DBG_LEVEL_ERROR,
          ("<%s> QCPNP_RegisterDevName failed to allocate an IRP\n", pDevExt->PortName)
       );
       return STATUS_UNSUCCESSFUL;
   }

   pIrp->AssociatedIrp.SystemBuffer = ExAllocatePool(NonPagedPool, 4096);
   if (pIrp->AssociatedIrp.SystemBuffer == NULL)
   {
      QCSER_DbgPrint
      (
         QCSER_DBG_MASK_CONTROL,
         QCSER_DBG_LEVEL_ERROR,
         ("<%s> QCPNP_RegisterDevName failed to allocate buffer\n", pDevExt->PortName)
      );
      IoFreeIrp(pIrp);
      return STATUS_UNSUCCESSFUL;
   }

   pDevInfoHdr = (PPEER_DEV_INFO_HDR)(pIrp->AssociatedIrp.SystemBuffer);
   pDevInfoHdr->Version = 1;
   pDevInfoHdr->DeviceNameLength = BufferLength;
   pDevInfoHdr->SymLinkNameLength = strlen(pDevExt->PortName);
   totalLength = sizeof(PEER_DEV_INFO_HDR) + pDevInfoHdr->DeviceNameLength +
                 pDevInfoHdr->SymLinkNameLength;

   pLocation = (PCHAR)(pIrp->AssociatedIrp.SystemBuffer);
   pLocation += sizeof(PEER_DEV_INFO_HDR);

   // device name
   RtlCopyMemory (pLocation, Buffer, BufferLength);

   // symbolic name
   pLocation += BufferLength;
   RtlCopyMemory(pLocation, pDevExt->PortName, pDevInfoHdr->SymLinkNameLength);

   QCUTIL_PrintBytes
   (
      (PVOID)(pIrp->AssociatedIrp.SystemBuffer),
      totalLength,
      totalLength,
      "PeerInfo",
      pDevExt,
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_DETAIL
   );

   // set the event
   pIrp->UserEvent = &Event;
   
   nextstack = IoGetNextIrpStackLocation(pIrp);
   nextstack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
   nextstack->Parameters.DeviceIoControl.IoControlCode = IoControlCode;
   nextstack->Parameters.DeviceIoControl.InputBufferLength = totalLength;

   IoSetCompletionRoutine
   (
      pIrp,
      (PIO_COMPLETION_ROUTINE)QCPNP_RegisterDevNameCompletion,
      (PVOID)pDevExt,
      TRUE,TRUE,TRUE
   );
   
   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_TRACE,
      ("<%s> QCPNP_RegisterDevName (IRP 0x%p)\n", pDevExt->PortName, pIrp)
   );

   Status = IoCallDriver(pDevExt->StackDeviceObject, pIrp);

   Status = KeWaitForSingleObject(&Event, Executive, KernelMode, TRUE, 0);

   if (Status == STATUS_SUCCESS)
   {
      Status = pIrp->IoStatus.Status;
   }

   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_TRACE,
      ("<%s> <--- QCPNP_RegisterDevName: ST %x\n", pDevExt->PortName, Status)
   );

   if (pIrp->AssociatedIrp.SystemBuffer != NULL)
   {
      ExFreePool(pIrp->AssociatedIrp.SystemBuffer);
   }
   IoFreeIrp(pIrp);
   
   return Status;
} // QCPNP_RegisterDevName

NTSTATUS QCPNP_RegisterDevNameCompletion
(
   PDEVICE_OBJECT pDO,
   PIRP           pIrp,
   PVOID          pContext
)
{
   PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pContext;

   QCSER_DbgPrint
   (
      QCSER_DBG_MASK_CONTROL,
      QCSER_DBG_LEVEL_TRACE,
      ("<%s> QCPNP_RegisterDevNameCompletion (IRP 0x%p IoStatus: 0x%x)\n", pDevExt->PortName, pIrp, pIrp->IoStatus.Status)
   );

   KeSetEvent(pIrp->UserEvent, 0, FALSE);

   return STATUS_MORE_PROCESSING_REQUIRED;
}  // QCPNP_RegisterDevNameCompletion

