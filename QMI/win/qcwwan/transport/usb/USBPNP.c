/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             U S B P N P . C

GENERAL DESCRIPTION
  This file implementations of PNP functions.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

//  Copyright 3Com Corporation 1997   All Rights Reserved.
//
                   
#include <stdarg.h>
#include <stdio.h>
#include "USBMAIN.h"
#include "USBUTL.h"
#include "USBINT.h"
#include "USBPNP.h"
#include "USBRD.h"
#include "USBWT.h"
#include "USBDSP.h"
#include "USBPWR.h"
#include "USBIOC.h"

#ifdef NDIS_WDM
#include "USBIF.h"
#endif

// EVENT_TRACING
#ifdef EVENT_TRACING

#include "MPWPP.h"               // Driver specific WPP Macros, Globals, etc 
#include "USBPNP.tmh"

#endif   // EVENT_TRACING

extern NTKERNELAPI VOID IoReuseIrp(IN OUT PIRP Irp, IN NTSTATUS Iostatus);

//{0x98b06a49, 0xb09e, 0x4896, {0x94, 0x46, 0xd9, 0x9a, 0x28, 0xca, 0x4e, 0x5d}};
GUID gQcFeatureGUID = {0x496ab098, 0x9eb0, 0x9648,  // in network byte order
                       {0x94, 0x46, 0xd9, 0x9a, 0x28, 0xca, 0x4e, 0x5d}};


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
#define QCUSB_StringForSysState(sysState) szSystemPowerState[sysState]
#define QCUSB_StringForDevState(devState) szDevicePowerState[devState]


#ifndef NDIS_WDM

NTSTATUS USBPNP_AddDevice
(
   IN PDRIVER_OBJECT pDriverObject,
   IN PDEVICE_OBJECT pdo
)
{
   char                  myPortName[16];
   NTSTATUS              ntStatus  = STATUS_SUCCESS;
   PDEVICE_OBJECT        fdo = NULL;
   PDEVICE_EXTENSION     pDevExt;

   bAddNewDevice = TRUE;
   QcAcquireEntryPass(&gSyncEntryEvent, "qc-add");

   ntStatus = QCUSB_VendorRegistryProcess(pDriverObject, pdo);
   if (!NT_SUCCESS(ntStatus))
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> AddDevice: reg access failure.\n", gDeviceName)
      );
      goto USBPNP__PnPAddDevice_Return;
   }

   QCUSB_DbgPrintG
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> AddDevice: <%d+%d>\n   DRVO=0x%p, PHYDEVO=0x%p\n", gDeviceName,
          sizeof(struct _DEVICE_OBJECT), sizeof(DEVICE_EXTENSION),
          pDriverObject, pdo)
   );

   strcpy(myPortName, "qcu");

   QCUSB_DbgPrintG2
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> AddDevice: Creating FDO...\n", myPortName)
   );

   ntStatus = IoCreateDevice
              (
                 pDriverObject,
                 sizeof(DEVICE_EXTENSION),
                 NULL,                // unnamed
                 FILE_DEVICE_NETWORK, // FILE_DEVICE_UNKNOWN
                 0, // FILE_DEVICE_SECURE_OPEN,  // DeviceCharacteristics
                 FALSE,
                 &fdo
              );

   if (NT_SUCCESS(ntStatus))
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> AddDevice: new FDO 0x%p\n", myPortName, fdo)
      );
      fdo->Flags |= DO_DIRECT_IO;
      fdo->Flags &= ~DO_EXCLUSIVE;
   }
   else
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_CRITICAL,
         ("<%s> AddDevice: FDO failure 0x%x\n", myPortName, ntStatus)
      );
      fdo = NULL;
      goto USBPNP__PnPAddDevice_Return;
   }

   pDevExt = (PDEVICE_EXTENSION)fdo->DeviceExtension;

   // initialize device extension
   ntStatus = USBPNP_InitDevExt(pDriverObject, 0, pdo, fdo, myPortName);
   if (!NT_SUCCESS(ntStatus))
   {
      QCUSB_CleanupDeviceExtensionBuffers(fdo);
      QcIoDeleteDevice(fdo);
      goto USBPNP__PnPAddDevice_Return;
   }

   // Attach FDO to the stack
   pDevExt->StackDeviceObject = IoAttachDeviceToDeviceStack(fdo, pdo);
   if (pDevExt->StackDeviceObject != NULL)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> AddDevice: Attached: lrPtr=0x%p upPtr=0x%p\n", myPortName,
          pDevExt->StackDeviceObject,
          pDevExt->StackDeviceObject->AttachedDevice)
      );
   }
   else
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_CRITICAL,
         ("<%s> AddDevice: IoAttachDeviceToDeviceStack failure\n", myPortName)
      );
      ntStatus = STATUS_UNSUCCESSFUL;
      QCUSB_CleanupDeviceExtensionBuffers(fdo);
      QcIoDeleteDevice(fdo);
      goto USBPNP__PnPAddDevice_Return;
   }

   // Continue initializing extension of the port DO
   pDevExt->bDeviceRemoved = FALSE;
   pDevExt->bDeviceSurpriseRemoved = FALSE;
   pDevExt->bmDevState = DEVICE_STATE_ZERO;

   USBPNP_GetDeviceCapabilities(pDevExt, bPowerManagement); // store info in portExt
   pDevExt->bPowerManagement = bPowerManagement;

   fdo->Flags |= DO_POWER_PAGABLE;
   fdo->Flags &= ~DO_DEVICE_INITIALIZING;

   ntStatus = USBDSP_InitDispatchThread(fdo);

   USBPNP__PnPAddDevice_Return:

   QcReleaseEntryPass(&gSyncEntryEvent, "qc-add", "END");
   bAddNewDevice = FALSE;

   return ntStatus;
}  // AddDevice

#endif // NDIS_WDM

//  Copyright 3Com Corporation 1997   All Rights Reserved.
//
//
//
//  09/29/97  Tim  Added Device Interface 'shingle' for CCPORT interface
//
//

NTSTATUS USBPNP_GetDeviceCapabilities
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
      // deviceExtension->DeviceCapabilities.SurpriseRemovalOK = TRUE;
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

      if (!bPowerManagement)
      {
         deviceExtension->DeviceCapabilities.DeviceState[i] = 0;
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

// #ifdef DEBUG_MSGS
   QCUSB_DbgPrintG
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("WakeFromD0/1/2/3 = %u, %u, %u, %u\n",
          deviceExtension->DeviceCapabilities.WakeFromD0,
          deviceExtension->DeviceCapabilities.WakeFromD1,
          deviceExtension->DeviceCapabilities.WakeFromD2,
          deviceExtension->DeviceCapabilities.WakeFromD3
      )
   );

   QCUSB_DbgPrintG
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("SystemWake = %s\n",
       QCUSB_StringForSysState(deviceExtension->DeviceCapabilities.SystemWake)
      )
   );
   QCUSB_DbgPrintG
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("DeviceWake = %s\n",
         QCUSB_StringForDevState(deviceExtension->DeviceCapabilities.DeviceWake)
      )
   );

   for (i=PowerSystemUnspecified; i< PowerSystemMaximum; i++)
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         (" Device State Map: sysstate %s = devstate %s\n",
          QCUSB_StringForSysState(i),
          QCUSB_StringForDevState(deviceExtension->DeviceCapabilities.DeviceState[i])
         )
      );
   }

   QCUSB_DbgPrintG
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("D1-D3 Latency = %u, %u, %u (x 100us)\n",
         deviceExtension->DeviceCapabilities.D1Latency,
         deviceExtension->DeviceCapabilities.D2Latency,
         deviceExtension->DeviceCapabilities.D3Latency
      )
   );
   QCUSB_DbgPrintG
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("DeviceCapabilities: PowerDownLevel = %s (PM %d)\n",
        QCUSB_StringForDevState(deviceExtension->PowerDownLevel), bPowerManagement)
   );
// #endif // DEBUG_MSGS

   return ntStatus;
}

NTSTATUS QCPNP_SetStamp
(
   PDEVICE_OBJECT PhysicalDeviceObject,
   HANDLE         hRegKey,
   BOOLEAN        Startup
)
{
   UNICODE_STRING ucValueName;
   NTSTATUS ntStatus = STATUS_SUCCESS;
   ULONG stampValue = 0;
   BOOLEAN bSelfOpen = FALSE;

   if (hRegKey == 0)
   {
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
      bSelfOpen = TRUE;
   }

   RtlInitUnicodeString(&ucValueName, VEN_DEV_TIME);
   if (Startup == TRUE)
   {
      stampValue = 1;
      ZwSetValueKey
      (
         hRegKey,
         &ucValueName,
         0,
         REG_DWORD,
         (PVOID)&stampValue,
         sizeof(ULONG)
      );
   }
   else
   {
      // ZwDeleteValueKey(hRegKey, &ucValueName);
      stampValue = 0;
      ZwSetValueKey
      (
         hRegKey,
         &ucValueName,
         0,
         REG_DWORD,
         (PVOID)&stampValue,
         sizeof(ULONG)
      );
   }

   if (bSelfOpen == TRUE)
   {
      ZwClose(hRegKey);
   }

   return STATUS_SUCCESS;
}  // QCPNP_SetStamp

NTSTATUS QCPNP_GetStringDescriptor
(
   PDEVICE_OBJECT DeviceObject,
   UCHAR          Index,
   USHORT         LanguageId,
   BOOLEAN        MatchPrefix
)
{
   PDEVICE_EXTENSION pDevExt;
   PUSB_STRING_DESCRIPTOR pSerNum;
   URB urb;
   NTSTATUS ntStatus;
   PCHAR pSerLoc = NULL;
   UNICODE_STRING ucValueName;
   HANDLE         hRegKey;
   INT            strLen;
   BOOLEAN        bSetEntry = FALSE;

   pDevExt = DeviceObject->DeviceExtension;

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> -->_GetStringDescriptor DO 0x%p idx %d\n", pDevExt->PortName, DeviceObject, Index)
   );

   if (Index == 0)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> <--_GetStringDescriptor: index is NULL\n", pDevExt->PortName)
      );
      goto UpdateRegistry;
   }

   pSerNum = (PUSB_STRING_DESCRIPTOR)(pDevExt->DevSerialNumber);
   RtlZeroMemory(pDevExt->DevSerialNumber, 256);

   UsbBuildGetDescriptorRequest
   (
      &urb,
      (USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
      USB_STRING_DESCRIPTOR_TYPE,
      Index,
      LanguageId,
      pSerNum,
      NULL,
      sizeof(USB_STRING_DESCRIPTOR_TYPE),
      NULL
   );

   ntStatus = QCUSB_CallUSBD(DeviceObject, &urb);
   if (!NT_SUCCESS(ntStatus))
   {
      goto UpdateRegistry;
   }

   UsbBuildGetDescriptorRequest
   (
      &urb,
      (USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
      USB_STRING_DESCRIPTOR_TYPE,
      Index,
      LanguageId,
      pSerNum,
      NULL,
      pSerNum->bLength,
      NULL
   );

   ntStatus = QCUSB_CallUSBD(DeviceObject, &urb);
   if (!NT_SUCCESS(ntStatus))
   {
      RtlZeroMemory(pDevExt->DevSerialNumber, 256);
   }
   else
   {
      USBUTL_PrintBytes
      (
         (PVOID)(pDevExt->DevSerialNumber),
         pSerNum->bLength,
         256,
         "SerialNumber",
         pDevExt,
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL
      );
   }

   if (!NT_SUCCESS(ntStatus))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> _GetStringDescriptor DO 0x%p failure NTS 0x%x\n", pDevExt->PortName,
           DeviceObject, ntStatus)
      );
      goto UpdateRegistry;
   }
   else
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> _GetStringDescriptor DO 0x%p NTS 0x%x (%dB)\n", pDevExt->PortName,
           DeviceObject, ntStatus, pSerNum->bLength)
      );
   }

   if (pSerNum->bLength > 2)
   {
      strLen = (INT)pSerNum->bLength -2;  // exclude 2 bytes in header
   }
   else
   {
      strLen = 0;
   }
   pSerLoc = (PCHAR)pSerNum->bString;
   bSetEntry = TRUE;

   // search for "_SN:"
   if ((MatchPrefix == TRUE) && (strLen > 0))
   {
      INT   idx, adjusted = 0;
      PCHAR p = pSerLoc;
      BOOLEAN bMatchFound = FALSE;

      for (idx = 0; idx < strLen; idx++)
      {
         if ((*p     == '_') && (*(p+1) == 0) &&
             (*(p+2) == 'S') && (*(p+3) == 0) &&
             (*(p+4) == 'N') && (*(p+5) == 0) &&
             (*(p+6) == ':') && (*(p+7) == 0))
         {
            pSerLoc = p + 8;
            adjusted += 8;
            bMatchFound = TRUE;
            break;
         }
         p++;
         adjusted++;
      }

      // Adjust length
      if (bMatchFound == TRUE)
      {
         INT tmpLen = strLen;

         tmpLen -= adjusted;
         p = pSerLoc;
         while (tmpLen > 0)
         {
            if (((*p == ' ') && (*(p+1) == 0)) ||  // space
                ((*p == '_') && (*(p+1) == 0)))    // or _ for another field
            {
               break;
            }
            else
            {
               p += 2;       // advance 1 unicode byte
               tmpLen -= 2;  // remaining string length
            }
         }
         strLen = (USHORT)(p - pSerLoc); // 18;
      }
      else
      {
         QCUSB_DbgPrint
         (
            QCUSB_DBG_MASK_CONTROL,
            QCUSB_DBG_LEVEL_TRACE,
            ("<%s> <--QDBPNP_GetDeviceSerialNumber: no SN found\n", pDevExt->PortName)
         );
         ntStatus = STATUS_UNSUCCESSFUL;
         bSetEntry = FALSE;
      }
   }

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> _GetDeviceSerialNumber: strLen %d\n", pDevExt->PortName, strLen)
   );

UpdateRegistry:

   // update registry
   ntStatus = IoOpenDeviceRegistryKey
              (
                 pDevExt->PhysicalDeviceObject,
                 PLUGPLAY_REGKEY_DRIVER,
                 KEY_ALL_ACCESS,
                 &hRegKey
              );
   if (!NT_SUCCESS(ntStatus))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> QDBPNP_GetDeviceSerialNumber: reg access failure 0x%x\n", pDevExt->PortName, ntStatus)
      );
      return ntStatus;
   }
   if (MatchPrefix == FALSE)
   {
      RtlInitUnicodeString(&ucValueName, VEN_DEV_SERNUM);
   }
   else
   {
      RtlInitUnicodeString(&ucValueName, VEN_DEV_MSM_SERNUM);
   }
   if (bSetEntry == TRUE)
   {
      ZwSetValueKey
      (
         hRegKey,
         &ucValueName,
         0,
         REG_SZ,
         (PVOID)pSerLoc,
         strLen
      );
   }
   else
   {
      ZwDeleteValueKey(hRegKey, &ucValueName);
   }
   ZwClose(hRegKey);

   return ntStatus;

} // QCPNP_GetStringDescriptor

NTSTATUS QCPNP_SetFunctionProtocol(PDEVICE_EXTENSION pDevExt, ULONG ProtocolCode)
{
   NTSTATUS       ntStatus;
   UNICODE_STRING ucValueName;
   HANDLE         hRegKey;

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> --> _SetFunctionProtocolnter 0x%x\n", pDevExt->PortName, ProtocolCode)
   );

   ntStatus = IoOpenDeviceRegistryKey
              (
                 pDevExt->PhysicalDeviceObject,
                 PLUGPLAY_REGKEY_DRIVER,
                 KEY_ALL_ACCESS,
                 &hRegKey
              );
   if (!NT_SUCCESS(ntStatus))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> <-- _SetFunctionProtocolnter: failed to open registry 0x%x\n", pDevExt->PortName, ntStatus)
      );
      return ntStatus;
   }

   RtlInitUnicodeString(&ucValueName, VEN_DEV_PROTOC);
   ntStatus = ZwSetValueKey
              (
                 hRegKey,
                 &ucValueName,
                 0,
                 REG_DWORD,
                 (PVOID)&ProtocolCode,
                 sizeof(ULONG)
              );
   ZwClose(hRegKey);

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> <-- _SetFunctionProtocolnter 0x%x ST 0x%x\n", pDevExt->PortName, ProtocolCode, ntStatus)
   );

   return ntStatus;
}  // QCPNP_SetFunctionProtocol

NTSTATUS QCPNP_GenericCompletion
(
   PDEVICE_OBJECT pDO,
   PIRP           pIrp,
   PVOID          pContext
)
{
   PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pContext;

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> QCPNP_GenericCompletion (IRP 0x%p IoStatus: 0x%x)\n", pDevExt->PortName, pIrp, pIrp->IoStatus.Status)
   );

   KeSetEvent(pIrp->UserEvent, 0, FALSE);

   return STATUS_MORE_PROCESSING_REQUIRED;
}  // QCPNP_GenericCompletion

NTSTATUS QCPNP_GetParentDeviceName(PDEVICE_EXTENSION pDevExt)
{
   CHAR parentDevName[MAX_NAME_LEN];
   PIRP pIrp = NULL;
   PIO_STACK_LOCATION nextstack;
   NTSTATUS ntStatus;
   KEVENT event;

   RtlZeroMemory(parentDevName, MAX_NAME_LEN);
   KeInitializeEvent(&event, SynchronizationEvent, FALSE);

   pIrp = IoAllocateIrp((CCHAR)(pDevExt->StackDeviceObject->StackSize+2), FALSE);
   if( pIrp == NULL )
   {
       QCUSB_DbgPrint
       (
          QCUSB_DBG_MASK_CONTROL,
          QCUSB_DBG_LEVEL_ERROR,
          ("<%s> QCPNP_GetParentDeviceName failed to allocate an IRP\n", pDevExt->PortName)
       );
       return STATUS_UNSUCCESSFUL;
   }

   pIrp->AssociatedIrp.SystemBuffer = parentDevName;
   pIrp->UserEvent = &event;

   nextstack = IoGetNextIrpStackLocation(pIrp);
   nextstack->MajorFunction = IRP_MJ_DEVICE_CONTROL;
   nextstack->Parameters.DeviceIoControl.IoControlCode = IOCTL_QCDEV_GET_PARENT_DEV_NAME;
   nextstack->Parameters.DeviceIoControl.OutputBufferLength = MAX_NAME_LEN;

   IoSetCompletionRoutine
   (
      pIrp,
      (PIO_COMPLETION_ROUTINE)QCPNP_GenericCompletion,
      (PVOID)pDevExt,
      TRUE,TRUE,TRUE
   );

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> QCPNP_GetParentDeviceName (IRP 0x%p)\n", pDevExt->PortName, pIrp)
   );

   ntStatus = IoCallDriver(pDevExt->StackDeviceObject, pIrp);

   ntStatus = KeWaitForSingleObject(&event, Executive, KernelMode, TRUE, 0);

   if (ntStatus == STATUS_SUCCESS)
   {
      ntStatus = pIrp->IoStatus.Status;
      QCPNP_SaveParentDeviceNameToRegistry(pDevExt, parentDevName, pIrp->IoStatus.Information);
   }
   else
   {
      QCPNP_SaveParentDeviceNameToRegistry(pDevExt, parentDevName, 0);
   }

   IoFreeIrp(pIrp);

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> <--- QCPNP_GetParentDeviceName: ST %x\n", pDevExt->PortName, ntStatus)
   );

   return ntStatus;

}  // QCPNP_GetParentDeviceName

VOID QCPNP_SaveParentDeviceNameToRegistry
(
   PDEVICE_EXTENSION pDevExt,
   PVOID ParentDeviceName,
   ULONG NameLength
)
{
   HANDLE hRegKey;
   NTSTATUS ntStatus;
   UNICODE_STRING ucValueName;

   // update registry
   ntStatus = IoOpenDeviceRegistryKey
              (
                 pDevExt->PhysicalDeviceObject,
                 PLUGPLAY_REGKEY_DRIVER,
                 KEY_ALL_ACCESS,
                 &hRegKey
              );
   if (!NT_SUCCESS(ntStatus))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> QCPNP_SaveParentDeviceNameToRegistry: reg access failure 0x%x\n", pDevExt->PortName, ntStatus)
      );
      return;
   }
   RtlInitUnicodeString(&ucValueName, L"QCDeviceParent");
   if (NameLength > 0)
   {
      ZwSetValueKey
      (
         hRegKey,
         &ucValueName,
         0,
         REG_SZ,
         ParentDeviceName,
         NameLength
      );
   }
   else
   {
      ZwDeleteValueKey(hRegKey, &ucValueName);
   }
   ZwClose(hRegKey);
}  // QCPNP_SaveParentDeviceNameToRegistry


#ifndef NDIS_WDM

NTSTATUS QCUSB_VendorRegistryProcess
(
   IN PDRIVER_OBJECT pDriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject
)
{
   NTSTATUS       ntStatus  = STATUS_SUCCESS;
   UNICODE_STRING ucDriverVersion, ucDriverVersion1;
   HANDLE         hRegKey = NULL;
   UNICODE_STRING ucValueName;
   ULONG          ulReturnLength;
   ULONG          gDriverConfigParam = 0;

   // Init default configuration
   gVendorConfig.ContinueOnOverflow  = FALSE;
   gVendorConfig.ContinueOnOverflow  = FALSE;
   gVendorConfig.ContinueOnDataError = FALSE;
   gVendorConfig.Use1600ByteInPkt    = FALSE;
   gVendorConfig.Use2048ByteInPkt    = FALSE;
   gVendorConfig.Use4096ByteInPkt    = FALSE;
   gVendorConfig.NoTimeoutOnCtlReq   = FALSE;
   gVendorConfig.EnableLogging       = FALSE;
   gVendorConfig.MinInPktSize        = 64;
   gVendorConfig.MaxPipeXferSize     = QCUSB_RECEIVE_BUFFER_SIZE;
   gVendorConfig.NumOfRetriesOnError = BEST_RETRIES;
   gVendorConfig.UseMultiReads       = TRUE;  // enabled by default
   gVendorConfig.UseMultiWrites      = TRUE;  // enabled by default

   ntStatus = IoOpenDeviceRegistryKey
              (
                 PhysicalDeviceObject,
                 PLUGPLAY_REGKEY_DRIVER,
                 KEY_ALL_ACCESS,
                 &hRegKey
              );
   if (!NT_SUCCESS(ntStatus))
   {
      // _failx;
      return ntStatus;
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
      gVendorConfig.MaxPipeXferSize = QCUSB_RECEIVE_BUFFER_SIZE;
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
      gVendorConfig.NumberOfL2Buffers = QCUSB_NUM_OF_LEVEL2_BUF;
   }
   else
   {
      if (gVendorConfig.NumberOfL2Buffers < 2)
      {
         gVendorConfig.NumberOfL2Buffers = 2;
      }
      else if (gVendorConfig.NumberOfL2Buffers > QCUSB_MAX_MRD_BUF_COUNT)
      {
         gVendorConfig.NumberOfL2Buffers = QCUSB_MAX_MRD_BUF_COUNT;
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
      gVendorConfig.DebugMask = QCUSB_DBG_LEVEL_FORCE;
   }
   #ifdef DEBUG_MSGS
   gVendorConfig.DebugMask = 0xFFFFFFFF;
   #endif
   gVendorConfig.DebugLevel = (UCHAR)(gVendorConfig.DebugMask & 0x0F);

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
      if (gDriverConfigParam & QCUSB_CONTINUE_ON_OVERFLOW)
      {
         gVendorConfig.ContinueOnOverflow = TRUE;
      }
      if (gDriverConfigParam & QCUSB_CONTINUE_ON_DATA_ERR)
      {
         gVendorConfig.ContinueOnDataError = TRUE;
      }
      if (gDriverConfigParam & QCUSB_USE_1600_BYTE_IN_PKT)
      {
         gVendorConfig.Use1600ByteInPkt = TRUE;
         gVendorConfig.MinInPktSize    = 1600;
      }
      if (gDriverConfigParam & QCUSB_USE_2048_BYTE_IN_PKT)
      {
         gVendorConfig.Use1024ByteInPkt = TRUE;
         gVendorConfig.MinInPktSize    = 2048;
      }
      if (gDriverConfigParam & QCUSB_USE_4096_BYTE_IN_PKT)
      {
         gVendorConfig.Use4096ByteInPkt = TRUE;
         gVendorConfig.MinInPktSize    = 4096;
      }
      if (gDriverConfigParam & QCUSB_NO_TIMEOUT_ON_CTL_REQ)
      {
         gVendorConfig.NoTimeoutOnCtlReq = TRUE;
      }
      if (gDriverConfigParam & QCUSB_ENABLE_LOGGING)
      {
         gVendorConfig.EnableLogging = TRUE;
      }
      // if (gDriverConfigParam & QCUSB_USE_MULTI_READS)
      // {
      //    gVendorConfig.UseMultiReads = TRUE;
      // }
      // if (gDriverConfigParam & QCUSB_USE_MULTI_WRITES)
      // {
      //    gVendorConfig.UseMultiWrites = TRUE;
      // }
   }

   // set values into range
   if (gVendorConfig.UseMultiReads == FALSE)
   {
      gVendorConfig.MaxPipeXferSize = QCUSB_MAX_PKT; // no longer configurable
/***
      gVendorConfig.MaxPipeXferSize = gVendorConfig.MaxPipeXferSize / 64 * 64;
      if (gVendorConfig.MaxPipeXferSize > 4096)
      {
         gVendorConfig.MaxPipeXferSize = 4096;
      }
      if (gVendorConfig.MaxPipeXferSize < 1600)
      {
         gVendorConfig.MaxPipeXferSize = 1600;
      }
      if (gVendorConfig.MaxPipeXferSize < gVendorConfig.MinInPktSize)
      {
         gVendorConfig.MaxPipeXferSize = gVendorConfig.MinInPktSize;
      }
***/
   }
   else
   {
      if (gVendorConfig.MaxPipeXferSize < QCUSB_MRECEIVE_BUFFER_SIZE)
      {
         gVendorConfig.MaxPipeXferSize = QCUSB_MRECEIVE_BUFFER_SIZE;
      }
      if (gVendorConfig.MaxPipeXferSize > QCUSB_MRECEIVE_MAX_BUFFER_SIZE)
      {
         gVendorConfig.MaxPipeXferSize = QCUSB_MRECEIVE_MAX_BUFFER_SIZE;
      }
   }

   // Get number of L2 buffers
   RtlInitUnicodeString(&ucValueName, VEN_DRV_THREAD_PRI);
   ntStatus = getRegDwValueEntryData
              (
                 hRegKey,
                 ucValueName.Buffer,
                 &gVendorConfig.ThreadPriority
              );

   if (ntStatus != STATUS_SUCCESS)
   {
      gVendorConfig.ThreadPriority = 26;
   }
   
   if ( gVendorConfig.ThreadPriority < 10)
   {
      gVendorConfig.ThreadPriority = 10;
   }
   if ( gVendorConfig.ThreadPriority > HIGH_PRIORITY)
   {
      gVendorConfig.ThreadPriority = HIGH_PRIORPTY;
   }

   #ifndef QCNET_WHQL
   if (gVendorConfig.DebugLevel > 0)
   {
      DbgPrint("<%s> Vendor Config Info ------ \n", gDeviceName);
      DbgPrint("    ContinueOnOverflow:  0x%x\n", gVendorConfig.ContinueOnOverflow);
      DbgPrint("    ContinueOnDataError: 0x%x\n", gVendorConfig.ContinueOnDataError);
      DbgPrint("    Use1600ByteInPkt:    0x%x\n", gVendorConfig.Use1600ByteInPkt);
      DbgPrint("    Use2048ByteInPkt:    0x%x\n", gVendorConfig.Use2048ByteInPkt);
      DbgPrint("    Use4096ByteInPkt:    0x%x\n", gVendorConfig.Use4096ByteInPkt);
      DbgPrint("    NoTimeoutOnCtlReq:   0x%x\n", gVendorConfig.NoTimeoutOnCtlReq);
      DbgPrint("    EnableLogging:       0x%x\n", gVendorConfig.EnableLogging);
      DbgPrint("    UseMultiReads        0x%x\n", gVendorConfig.UseMultiReads);
      DbgPrint("    UseMultiWrites       0x%x\n", gVendorConfig.UseMultiWrites);
      DbgPrint("    MinInPktSize:        %ld\n",  gVendorConfig.MinInPktSize);
      DbgPrint("    MaxPipeXferSize      %ld\n",  gVendorConfig.MaxPipeXferSize);
      DbgPrint("    NumOfRetriesOnError: %ld\n",  gVendorConfig.NumOfRetriesOnError);
      DbgPrint("    NumberOfL2Buffers:   %ld\n",  gVendorConfig.NumberOfL2Buffers);
      DbgPrint("    DebugMask:           0x%x\n", gVendorConfig.DebugMask);
      DbgPrint("    DebugLevel:          %ld\n",  gVendorConfig.DebugLevel);
      DbgPrint("    ThreadPriority       %ld\n",  gVendorConfig.ThreadPriority);
   }
   #endif // QCNET_WHQL

   _closeRegKeyG(gDeviceName, hRegKey, "PNP-2");
   _freeString(ucDriverVersion);

   return STATUS_SUCCESS;
} // QCUSB_VendorRegistryProcess

NTSTATUS QCUSB_PostVendorRegistryProcess
(
   IN PDRIVER_OBJECT pDriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject,
   IN PDEVICE_OBJECT DeviceObject
)
{
   NTSTATUS       ntStatus  = STATUS_SUCCESS;
   HANDLE         hRegKey = NULL;
   UNICODE_STRING ucValueName;
   ULONG          ulReturnLength;
   PDEVICE_EXTENSION pDevExt = DeviceObject->DeviceExtension;

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
      // _failx;
      return ntStatus;
   }

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

   _closeRegKey( hRegKey, "PNP-3" );

   return STATUS_SUCCESS;
}  // QCUSB_PostVendorRegistryProcess

#endif // NDIS_WDM

NTSTATUS USBPNP_StartDevice( IN  PDEVICE_OBJECT DeviceObject, IN UCHAR cookie )
{
   PDEVICE_EXTENSION pDevExt;
   NTSTATUS ntStatus;
   PURB pUrb;
   USHORT j;
   ULONG siz, i;
   PUSB_DEVICE_DESCRIPTOR deviceDesc = NULL;
   ANSI_STRING asUsbPath;
   UCHAR ConfigIndex;

   pDevExt = DeviceObject -> DeviceExtension;
   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> StartDevice enter-%d: fdo 0x%p ext 0x%p\n", pDevExt->PortName, cookie,
        DeviceObject, pDevExt)
   );

   setDevState(DEVICE_STATE_PRESENT);

   if (pDevExt->bFdoReused == TRUE)
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

      if (pDevExt->UsbConfigUrb != NULL)
      {
         ExFreePool(pDevExt->UsbConfigUrb);
         pDevExt->UsbConfigUrb = NULL;
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

// #ifdef QCUSB_DBGPRINT2
            DbgPrint
            ("Device Descriptor = %p, len %u\n",
             deviceDesc,
             pUrb->UrbControlDescriptorRequest.TransferBufferLength
            );

            DbgPrint("Sample Device Descriptor:\n");
            DbgPrint("-------------------------\n");
            DbgPrint("bLength %d\n", deviceDesc->bLength);
            DbgPrint("bDescriptorType 0x%x\n", deviceDesc->bDescriptorType);
            DbgPrint("bcdUSB 0x%x\n", deviceDesc->bcdUSB);
            DbgPrint("bDeviceClass 0x%x\n", deviceDesc->bDeviceClass);
            DbgPrint("bDeviceSubClass 0x%x\n", deviceDesc->bDeviceSubClass);
            DbgPrint("bDeviceProtocol 0x%x\n", deviceDesc->bDeviceProtocol);
            DbgPrint("bMaxPacketSize0 0x%x\n", deviceDesc->bMaxPacketSize0);
            DbgPrint("idVendor 0x%x\n", deviceDesc->idVendor);
            DbgPrint("idProduct 0x%x\n", deviceDesc->idProduct);
            DbgPrint("bcdDevice 0x%x\n", deviceDesc->bcdDevice);
            DbgPrint("iManufacturer 0x%x\n", deviceDesc->iManufacturer);
            DbgPrint("iProduct 0x%x\n", deviceDesc->iProduct);
            DbgPrint("iSerialNumber 0x%x\n", deviceDesc->iSerialNumber);
            DbgPrint("bNumConfigurations 0x%x\n", deviceDesc->bNumConfigurations);
// #endif  //QCUSB_DBGPRINT2

            QCPNP_GetParentDeviceName(pDevExt);

            ntStatus = QCPNP_GetStringDescriptor(DeviceObject, deviceDesc->iProduct, 0x0409, TRUE);
            if ((!NT_SUCCESS(ntStatus)) && (deviceDesc->iProduct != 2))
            {
               QCUSB_DbgPrint
               (
                  QCUSB_DBG_MASK_CONTROL,
                  QCUSB_DBG_LEVEL_ERROR,
                  ("<%s> StartDevice: _SERN: Failed with iProduct: 0x%x, try default\n",
                    pDevExt->PortName, deviceDesc->iProduct)
               );
               // workaround: try default iProduct value 0x02
               ntStatus = QCPNP_GetStringDescriptor(DeviceObject, 0x02, 0x0409, TRUE);
            }
            QCUSB_DbgPrint
            (
               QCUSB_DBG_MASK_CONTROL,
               QCUSB_DBG_LEVEL_ERROR,
               ("<%s> StartDevice: _SERN: tried iProduct: ST(0x%x)\n",
                 pDevExt->PortName, ntStatus)
            );

            ntStatus = QCPNP_GetStringDescriptor(DeviceObject, deviceDesc->iSerialNumber, 0x0409, FALSE);
            QCUSB_DbgPrint
            (
               QCUSB_DBG_MASK_CONTROL,
               QCUSB_DBG_LEVEL_ERROR,
               ("<%s> StartDevice: _SERN: tried iSerialNumber: 0x%x ST(0x%x)\n",
                 pDevExt->PortName, deviceDesc->iSerialNumber, ntStatus)
            );
            ntStatus = STATUS_SUCCESS; // make possible failure none-critical
            ExFreePool( pUrb );
            pUrb = NULL;
            pDevExt -> idVendor = deviceDesc  -> idVendor;
            pDevExt -> idProduct = deviceDesc -> idProduct;

            if (deviceDesc->bcdUSB == QC_HSUSB_VERSION)
            {
               pDevExt->HighSpeedUsbOk |= QC_HSUSB_VERSION_OK;
            }
            if (deviceDesc->bcdUSB >= QC_SSUSB_VERSION)
            {
               pDevExt->HighSpeedUsbOk |= QC_SSUSB_VERSION_OK;
            }

            if (FALSE == USBPNP_ValidateDeviceDescriptor(pDevExt, pDevExt->pUsbDevDesc))
            {
               ntStatus = STATUS_UNSUCCESSFUL;
            }
         }
         else
         {
            if (ntStatus == STATUS_DEVICE_NOT_READY)
            {
               QCUSB_DbgPrint
               (
                  QCUSB_DBG_MASK_CONTROL,
                  QCUSB_DBG_LEVEL_CRITICAL,
                  ("<%s> StartDevice: dev not ready\n", pDevExt->PortName)
               );
               // IoInvalidateDeviceState(pDevExt->PhysicalDeviceObject);
            }
            else
            {
               QCUSB_DbgPrint
               (
                  QCUSB_DBG_MASK_CONTROL,
                  QCUSB_DBG_LEVEL_CRITICAL,
                  ("<%s> StartDevice: dev err-0 0x%x\n", pDevExt->PortName, ntStatus)
               );
            }
         }
      }  // if (deviceDesc)
      else
      {
         ntStatus = STATUS_NO_MEMORY;
         QCUSB_DbgPrint
         (
            QCUSB_DBG_MASK_CONTROL,
            QCUSB_DBG_LEVEL_CRITICAL,
            ("<%s> StartDevice: STATUS_NO_MEMORY-0\n", pDevExt->PortName)
         );
      }
   }
   else
   {
      ntStatus = STATUS_NO_MEMORY;
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_CRITICAL,
         ("<%s> StartDevice: STATUS_NO_MEMORY-1\n", pDevExt->PortName)
      );
   }

   if (!NT_SUCCESS( ntStatus ))
   {
      if (pUrb != NULL)
      {
         ExFreePool(pUrb);
      }
      goto StartDevice_Return;         
   }

   for (ConfigIndex = 0; ConfigIndex < deviceDesc->bNumConfigurations;
        ConfigIndex++)
   {
      ntStatus = USBPNP_ConfigureDevice( DeviceObject, ConfigIndex );
      if (NT_SUCCESS(ntStatus))
      {
         break;
      }
   }

   if (!NT_SUCCESS(ntStatus)) 
   {
      // _failx;
      goto StartDevice_Return;
   }              

   pDevExt->bDeviceRemoved = FALSE;
   pDevExt->bDeviceSurpriseRemoved = FALSE;

   if ((pDevExt->MuxInterface.MuxEnabled != 0x01) || 
       (pDevExt->MuxInterface.InterfaceNumber == pDevExt->MuxInterface.PhysicalInterfaceNumber))
   {
   for ( i = 0; i < 10; i++ )
   {
      QCUSB_ResetInt(DeviceObject, QCUSB_RESET_PIPE_AND_ENDPOINT);
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
      
         QCUSB_DbgPrint
         (
            QCUSB_DBG_MASK_CONTROL,
            QCUSB_DBG_LEVEL_ERROR,
            ("<%s> StartDevice: ResetInput/Output ResetPort %d err 0x%x\n", pDevExt->PortName, i, ntStatus)
         );
         
         // Sleep for 0.4sec
         KeDelayExecutionThread( KernelMode, FALSE, &timeoutValue );
      }
   }
   }

   if (!NT_SUCCESS(ntStatus)) 
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> StartDevice: ResetInput/Output ResetPort failed ALL RETRIES err 0x%x\n", pDevExt->PortName, ntStatus)
      );
      goto StartDevice_Return;
   }

   if (TRUE == USBIF_IsUsbBroken(DeviceObject))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_FORCE,
         ("<%s> StartDevice: Broken USB detected, fail\n", pDevExt->PortName)
      );
      ntStatus = STATUS_DELETE_PENDING;
      goto StartDevice_Return;
   }

#ifdef QCUSB_SHARE_INTERRUPT_OLD
   ntStatus = USBCTL_RequestDeviceID(DeviceObject, pDevExt->DataInterface);
   if (!NT_SUCCESS(ntStatus))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> StartDevice: DeviceId err 0x%x\n", pDevExt->PortName, ntStatus)
      );
      goto StartDevice_Return;
   }
#endif

   if ((pDevExt->MuxInterface.MuxEnabled != 0x01) || 
       (pDevExt->MuxInterface.InterfaceNumber == pDevExt->MuxInterface.PhysicalInterfaceNumber))
   {
   ntStatus = USBCTL_EnableDataBundling
              (
                 DeviceObject,
                 pDevExt->DataInterface,
                 QC_LINK_DL
              );
   if (!NT_SUCCESS(ntStatus))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> StartDevice: DL bundling err 0x%x\n", pDevExt->PortName, ntStatus)
      );
   }
   }
   // Initialize L2 Buffers
   ntStatus = USBRD_InitializeL2Buffers(pDevExt);
   if (!NT_SUCCESS(ntStatus))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_FORCE,
         ("<%s> StartDevice: L2 NO MEM\n", pDevExt->PortName)
      );
      goto StartDevice_Return;
   }

   // Now, bring up interrupt listener
   KeClearEvent(&pDevExt->CancelInterruptPipeEvent);
   ntStatus = USBINT_InitInterruptPipe(DeviceObject);
   if (pDevExt->InterruptPipe == (UCHAR)-1)
   {
      if(!NT_SUCCESS(ntStatus)) 
      {
         ntStatus = STATUS_SUCCESS;
      }              
   }
   
   if(NT_SUCCESS(ntStatus)) 
   {
      // pDevExt->bInService = FALSE;
   }

StartDevice_Return:

   if (!NT_SUCCESS(ntStatus))
   {
      clearDevState(DEVICE_STATE_PRESENT_AND_STARTED);
      USBRD_CancelReadThread(pDevExt, 3);
      USBWT_CancelWriteThread(pDevExt, 3);
      USBDSP_CancelDispatchThread(pDevExt, 2);
      USBINT_CancelInterruptThread(pDevExt, 1);
      // QCUSB_CleanupDeviceExtensionBuffers(DeviceObject);
   }
   else
   {
      QCPNP_SetStamp(pDevExt->PhysicalDeviceObject, 0, 1);
   }
                  
   return ntStatus;
}  //_StartDevice

NTSTATUS USBPNP_ConfigureDevice
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

   if (FALSE == USBPNP_ValidateConfigDescriptor(pDevExt, pConfigDesc))
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

   if (FALSE == USBPNP_ValidateConfigDescriptor(pDevExt, pConfigDesc))
   {
      ntStatus = STATUS_UNSUCCESSFUL;
      if (pConfigDesc != NULL)
      {
         ExFreePool(pConfigDesc);
      }
      pDevExt->pUsbConfigDesc = NULL;
      return ntStatus;
   }

   #ifdef QCUSB_DBGPRINT2
   {
      int i;
      p = (UCHAR*)pConfigDesc;
      for (i=0; i < ulSize; i++)
      {
         DbgPrint(" (%d, 0x%x)-", i, p[i]);
      }
      DbgPrint("\n");
   }
   #endif // QCUSB_DBGPRINT2

   bmAttr = (UCHAR)pConfigDesc->bmAttributes;
   pDevExt->bmAttributes = bmAttr;
   ntStatus = USBPNP_SelectInterfaces(DeviceObject, pConfigDesc);

   // TODO: Check for primary adapter can be a small function
   if ((pDevExt->MuxInterface.MuxEnabled != 0x01) || 
       (pDevExt->MuxInterface.InterfaceNumber == pDevExt->MuxInterface.PhysicalInterfaceNumber))
   {
   if (NT_SUCCESS(ntStatus) && ((bmAttr & REMOTE_WAKEUP_MASK) != 0))
   {
      // Set remote wakeup feature, we don't care about the status
      QCUSB_ClearRemoteWakeup(DeviceObject);
   }
   }
   if ( bmAttr & SELF_POWERED_MASK )
   {
      pDevExt->bDeviceSelfPowered = TRUE;
   }
   
   return ntStatus;
}  //USBPNP_ConfigureDevice

NTSTATUS USBPNP_SelectInterfaces
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
   PUSBD_INTERFACE_INFORMATION pInterfaceInfo[MAX_INTERFACE], pIntinfo = NULL;
   PUSBD_PIPE_INFORMATION pipeInformation;
   UCHAR ucPipeIndex;
   LONG lCommClassInterface = -1, lDataClassInterface = -1; // CDC only
   USBD_INTERFACE_LIST_ENTRY InterfaceList[MAX_INTERFACE];
   PVOID pStartPosition;
   PCDCC_FUNCTIONAL_DESCRIPTOR pFuncd;
   UCHAR *p = (UCHAR *)pConfigDesc;
   UCHAR *pDescEnd;
   GUID *featureGuid;
   /*****
      typedef struct {
          unsigned long  Data1;
          unsigned short Data2;
          unsigned short Data3;
          unsigned char  Data4[8];
      } GUID;
   *****/

   pDevExt = pDevObj -> DeviceExtension;

   pDevExt->IsEcmModel = FALSE;
   pDevExt->BulkPipeInput  =
   pDevExt->BulkPipeOutput =
   pDevExt->InterruptPipe  = (UCHAR)-1;

   pDevExt->ControlInterface = pDevExt->DataInterface = 0;

   lTotalInterfaces = pConfigDesc->bNumInterfaces;

   #ifdef QCUSB_DBGPRINT2
   DbgPrint("selectinterface: Cfg->bLength=0x%x\n", (UCHAR)pConfigDesc->bLength);
   DbgPrint("selectinterface: Cfg->bDescType=0x%x\n", (UCHAR)pConfigDesc->bDescriptorType);
   DbgPrint("selectinterface: Cfg->wTotalLen=0x%x\n", (USHORT)pConfigDesc->wTotalLength);
   DbgPrint("selectinterface: Cfg->bNumInfs=0x%x\n", (UCHAR)lTotalInterfaces);
   DbgPrint("selectinterface: Cfg->bmAttr=0x%x\n", (UCHAR)pConfigDesc->bmAttributes);
   DbgPrint("selectinterface: Cfg->MaxPower=0x%x\n", (UCHAR)pConfigDesc->MaxPower);
   #endif // QCUSB_DBGPRINT2

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
            QCUSB_DbgPrint
            (
               QCUSB_DBG_MASK_READ,
               QCUSB_DBG_LEVEL_CRITICAL,
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
            QCUSB_DbgPrint
            (
               QCUSB_DBG_MASK_READ,
               QCUSB_DBG_LEVEL_CRITICAL,
               ("<%s>: CtrlCap 0x%x DataCap 0x%x\n", pDevExt->PortName,
                 bmControlCapabilities, bmDataCapabilities)
            );
            pDevExt->DeviceControlCapabilities = bmControlCapabilities;
            pDevExt->DeviceDataCapabilities    = bmDataCapabilities;
         }
      }
      // else if ((p[1] == USB_CDC_CS_INTERFACE) && (p[2] == USB_CDC_ACM_FD))
      // {
      //    UCHAR bmCapabilities = p[3];

      //    if ((USB_CDC_ACM_SET_COMM_FEATURE_BIT_MASK & bmCapabilities) != 0)
      //    {
      //       pDevExt->SetCommFeatureSupported = TRUE;
      //    }
      // }
      p += (UCHAR)*p;  // skip current descriptor
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
         QCUSB_DbgPrint
         (
            QCUSB_DBG_MASK_READ,
            QCUSB_DBG_LEVEL_CRITICAL,
            ("<%s> _SelectInterfaces: parse done, totalIF=%d\n",
              pDevExt->PortName, lTotalInterfaces)
         );
      }
      else
      {
         #ifdef QCUSB_DBGPRINT2
         DbgPrint(" --- SEARCH %d ---\n", x-1);
         DbgPrint(" - INF.bLength = 0x%x\n", (UCHAR)pIntdesc->bLength);
         DbgPrint(" - INF.bDescriptorType = 0x%x\n", (UCHAR)pIntdesc->bDescriptorType);
         DbgPrint(" - INF.bInterfaceNumber = 0x%x\n", (UCHAR)pIntdesc->bInterfaceNumber);
         DbgPrint(" - INF.bAlternateSetting = 0x%x\n", (UCHAR)pIntdesc->bAlternateSetting);
         DbgPrint(" - INF.bNumEndpoints = 0x%x\n", (UCHAR)pIntdesc->bNumEndpoints);
         DbgPrint(" - INF.bInterfaceClass = 0x%x\n", (UCHAR)pIntdesc->bInterfaceClass);
         DbgPrint(" - INF.bInterfaceSubClass = 0x%x\n", (UCHAR)pIntdesc->bInterfaceSubClass);
         DbgPrint(" - INF.bInterfaceProtocol = 0x%x\n", (UCHAR)pIntdesc->bInterfaceProtocol);
         #endif // QCUSB_DBGPRINT2

         pDevExt->IfProtocol = (ULONG)pIntdesc->bInterfaceProtocol        |
                               ((ULONG)pIntdesc->bInterfaceClass)   << 8  |
                               ((ULONG)pIntdesc->bAlternateSetting) << 16 |
                               ((ULONG)pIntdesc->bInterfaceNumber)  << 24;

         // to identify if it's an ECM model
         if ((x == 0) &&
             (pIntdesc->bInterfaceClass == CDCC_COMMUNICATION_INTERFACE_CLASS) &&
             (pIntdesc->bInterfaceSubClass == CDCC_ECM_INTERFACE_CLASS) &&
             (pIntdesc->bAlternateSetting == 0) &&
             (pIntdesc->bNumEndpoints == 1)) // interrupt EP
         {
            pDevExt->IsEcmModel = TRUE;
         }

         if ((pDevExt->IsEcmModel == TRUE) && (x > 0))
         {
            if ((pIntdesc->bAlternateSetting == 0) &&
                (pIntdesc->bInterfaceClass == CDCC_DATA_INTERFACE_CLASS) &&
                (pIntdesc->bInterfaceSubClass == 0) &&
                (pIntdesc->bInterfaceProtocol == 0))
            {
               QCUSB_DbgPrint
               (
                  QCUSB_DBG_MASK_CONTROL,
                  QCUSB_DBG_LEVEL_DETAIL,
                  ("<%s> _SelectInterfaces: ignore ECM reset IF (%u EP)\n",
                    pDevExt->PortName, pIntdesc->bNumEndpoints)
               );

               // select alternate setting 0 initially to disable the data IF
               // InterfaceList[x++].InterfaceDescriptor = pIntdesc;
            }
            if ((pIntdesc->bAlternateSetting != 0) &&
                (pIntdesc->bInterfaceClass == CDCC_DATA_INTERFACE_CLASS) &&
                (pIntdesc->bInterfaceSubClass == 0) &&
                (pIntdesc->bInterfaceProtocol == 0))
            {
               QCUSB_DbgPrint
               (
                  QCUSB_DBG_MASK_CONTROL,
                  QCUSB_DBG_LEVEL_DETAIL,
                  ("<%s> _SelectInterfaces-a: IF %u alt %u added to config[%u] (%u EP)\n",
                    pDevExt->PortName, pIntdesc->bInterfaceNumber, pIntdesc->bAlternateSetting,
                    x, pIntdesc->bNumEndpoints)
               );

               // Select alternate setting 1 initially to enable the data IF
               InterfaceList[x++].InterfaceDescriptor = pIntdesc;
               pDevExt->HighSpeedUsbOk |= QC_HSUSB_ALT_SETTING_OK;
            }
         }
         else
         {
            pDevExt->SetCommFeatureSupported = TRUE;

            if ((pIntdesc->bAlternateSetting != 0) && (pIntdesc->bNumEndpoints > 1))
            {
               // For HS-USB vendor-specific descriptors with alter_settings
               // Overwrite the previous one
               InterfaceList[--x].InterfaceDescriptor = pIntdesc;
               pDevExt->SetCommFeatureSupported = FALSE;
               pDevExt->HighSpeedUsbOk |= QC_HSUSB_ALT_SETTING_OK;
            }
            else
            {
               InterfaceList[x++].InterfaceDescriptor = pIntdesc;
            }
            QCUSB_DbgPrint
            (
               QCUSB_DBG_MASK_CONTROL,
               QCUSB_DBG_LEVEL_DETAIL,
               ("<%s> _SelectInterfaces-b: IF %u alt %u added to config[%u] (%u EP)\n",
                 pDevExt->PortName, pIntdesc->bInterfaceNumber, pIntdesc->bAlternateSetting,
                 x, pIntdesc->bNumEndpoints)
            );

            QCPNP_SetFunctionProtocol(pDevExt, pDevExt->IfProtocol);
         }

         if (x >= MAX_INTERFACE)
         {
            QCUSB_DbgPrint
            (
               QCUSB_DBG_MASK_CONTROL,
               QCUSB_DBG_LEVEL_CRITICAL,
               ("<%s> _SelectInterfaces: error -- too many interfaces\n", pDevExt->PortName)
            );
            return STATUS_UNSUCCESSFUL;
         }
         pStartPosition = (PVOID)((PCHAR)pIntdesc + pIntdesc->bLength);
      }
   }  // while

   pUrb = USBD_CreateConfigurationRequestEx( pConfigDesc, InterfaceList );
   if ((pUrb == NULL)||(pUrb == (PURB)(-1)))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_READ,
         QCUSB_DBG_LEVEL_CRITICAL,
         ("<%s> _SelectInterfaces: err - USBD_CreateConfigurationRequestEx\n",
           pDevExt->PortName)
      );
      return STATUS_NO_MEMORY;
   }
   pDevExt->UsbConfigUrb = pUrb;

   for (i = 0; i < MAX_INTERFACE; i++)
   {
      #ifdef QCUSB_DBGPRINT2
      if (InterfaceList[i].Interface == NULL)
      {
         DbgPrint(" <%d> - Interface is NULL \n", i);
      }
      else
      {
         DbgPrint(" <%d> - Interface is NOT NULL!!! \n", i);
      }
      #endif // QCUSB_DBGPRINT2

      if (InterfaceList[i].InterfaceDescriptor == NULL)
      {
         #ifdef QCUSB_DBGPRINT2
         DbgPrint(" <%d> - InterfaceDesc is NULL \n", i);
         #endif
         continue;
      }
      
      pIntdesc = InterfaceList[i].InterfaceDescriptor;
      pIntinfo = InterfaceList[i].Interface;
      if (pIntinfo == NULL)
      {
         QCUSB_DbgPrint
         (
            QCUSB_DBG_MASK_CONTROL,
            QCUSB_DBG_LEVEL_ERROR,
            (" pIntinfo is NULL, FAILED!!!\n")
         );
         ExFreePool( pUrb );
         pDevExt->UsbConfigUrb = NULL;
         return STATUS_NO_MEMORY;
      }

      for (x = 0; x < pIntinfo->NumberOfPipes; x++)
      {
         pIntinfo->Pipes[x].MaximumTransferSize = pDevExt->MaxPipeXferSize;
      }
      ucNumPipes += pIntinfo -> NumberOfPipes;
      pIntinfo -> Length = GET_USBD_INTERFACE_SIZE( pIntdesc -> bNumEndpoints );
      pIntinfo -> InterfaceNumber = pIntdesc -> bInterfaceNumber;
      pIntinfo -> AlternateSetting = pIntdesc -> bAlternateSetting;

   } // for (interface list[i++])

   if (ucNumPipes > MAX_IO_QUEUES)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _SelectInterfaces: Too many pipes, %d\n", pDevExt->PortName, ucNumPipes)
      );
      return STATUS_INSUFFICIENT_RESOURCES;
   }
   QCUSB_DbgPrint2
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_ERROR,
      ("<%s> Conf URB Length: %d, Function: %x\n", pDevExt->PortName,
          pUrb -> UrbSelectConfiguration.Hdr.Length,
          pUrb -> UrbSelectConfiguration.Hdr.Function)
   );

   QCUSB_DbgPrint2
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_ERROR,
      ("<%s> ConfigurationDesc: %p, ConfigurationHandle: %p\n", pDevExt->PortName,
         pUrb -> UrbSelectConfiguration.ConfigurationDescriptor,
         pUrb -> UrbSelectConfiguration.ConfigurationHandle)
   );

   ntStatus = QCUSB_CallUSBD( pDevObj, pUrb );

   #ifdef QCUSB_DBGPRINT2
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
      DbgPrint(" CONFIG Purb NTSTATUS_SUCCESS\n\n");
   }
   else
   {
      DbgPrint(" CONFIG URB Failed NTSTATUS = 0x%x...\n\n",
                   pUrb->UrbSelectConfiguration.Hdr.Status);
   }
   #endif // QCUSB_DBGPRINT2

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
                     pDevExt -> wMaxPktSize =
                        pipeInformation -> MaximumPacketSize;
                     if (pDevExt->BulkPipeOutput == (UCHAR) -1) //take the 1st
                     {
                        pDevExt->DataInterface = pIntinfo->InterfaceNumber; // x;
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
                     // pDevExt->ControlInterface = pIntinfo->InterfaceNumber;
                     pDevExt -> InterruptPipe = ucPipeIndex;
                  }
               } //if

               #ifdef QCUSB_DBGPRINT2
               // Dump the pipe info
               DbgPrint("---------\n");
               DbgPrint("PipeType 0x%x\n", pipeInformation->PipeType);
               DbgPrint("EndpointAddress 0x%x\n", pipeInformation->EndpointAddress);
               DbgPrint("MaxPacketSize 0x%x\n", pipeInformation->MaximumPacketSize);
               DbgPrint("Interval 0x%x\n", pipeInformation->Interval);
               DbgPrint("Handle %p\n", pipeInformation->PipeHandle);
               #endif // QCUSB_DBGPRINT2
            } //for
         } //if
         else
         {
            QCUSB_DbgPrint
            (
               QCUSB_DBG_MASK_CONTROL,
               QCUSB_DBG_LEVEL_CRITICAL,
               ("<%s> _SelectInterfaces: no mem - if\n", pDevExt->PortName)
            );
            ntStatus = STATUS_NO_MEMORY;
            break;
         }
      }  // for loop
      // ExFreePool( pUrb );
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
             pDevExt->ucModelType = MODELTYPE_NET;
             gModemType = MODELTYPE_NET;
             #ifndef QCNET_WHQL
             DbgPrint("\n   ============ Loaded ==========\n");
             DbgPrint("   | Device Type: CDC(%02X)       |\n", pDevExt->DataInterface);
             DbgPrint("   |   Version: %-10s      |\n", gVendorConfig.DriverVersion);
             DbgPrint("   |   Device:  %-10s      |\n", pDevExt->PortName);
             DbgPrint("   |   IF: CT%02d-CC%02d-DA%02d       |\n", pDevExt->ControlInterface,
                       pDevExt->usCommClassInterface, pDevExt->DataInterface);
             DbgPrint("   |   EP(0x%x, 0x%x, 0x%x) HS 0x%x  ATTR : 0x%x |\n",
                         pDevExt->Interface[pDevExt->DataInterface]
                            ->Pipes[pDevExt->BulkPipeInput].EndpointAddress,
                         pDevExt->Interface[pDevExt->DataInterface]
                            ->Pipes[pDevExt->BulkPipeOutput].EndpointAddress,
                         pDevExt->Interface[pDevExt->usCommClassInterface]
                            ->Pipes[pDevExt->InterruptPipe].EndpointAddress,
                         pDevExt->HighSpeedUsbOk, pDevExt->bmAttributes);
             DbgPrint("Driver Version %s\n", "4.0.5.4");
             DbgPrint("   |============================|\n");
             #endif // QCNET_WHQL
          }
          else
          {
             pDevExt->ucModelType = MODELTYPE_NET_LIKE;
             gModemType = MODELTYPE_NET_LIKE;
             #ifndef QCNET_WHQL
             DbgPrint("\n   ============== Loaded ===========\n");
             DbgPrint("   | DeviceType: NET0(%02X)          |\n", pDevExt->DataInterface);
             DbgPrint("   |   Version: %-10s         |\n", gVendorConfig.DriverVersion);
             DbgPrint("   |   Device:  %-10s         |\n", pDevExt->PortName);
             DbgPrint("   |   IF: CT%02d-CC%02d-DA%02d          |\n", pDevExt->ControlInterface,
                       pDevExt->usCommClassInterface, pDevExt->DataInterface);
             DbgPrint("   |   EP(0x%x, 0x%x) HS 0x%x  ATTR : 0x%x |\n",
                         pDevExt->Interface[pDevExt->DataInterface]
                            ->Pipes[pDevExt->BulkPipeInput].EndpointAddress,
                         pDevExt->Interface[pDevExt->DataInterface]
                            ->Pipes[pDevExt->BulkPipeOutput].EndpointAddress,
                         pDevExt->HighSpeedUsbOk, pDevExt->bmAttributes);
             DbgPrint("Driver Version %s\n", "4.0.5.4");
             DbgPrint("   |===============================|\n");
             #endif // QCNET_WHQL
          }
          #ifndef QCNET_WHQL
          QCUSB_DbgPrint
          (
             QCUSB_DBG_MASK_CONTROL,
             QCUSB_DBG_LEVEL_INFO,
             ("     MaxPipeXfer: %u Bytes\n\n", pDevExt->MaxPipeXferSize)
          );
          #endif // QCNET_WHQL
      }
      else
      {
         ntStatus = STATUS_INSUFFICIENT_RESOURCES;
         #ifndef QCNET_WHQL
         DbgPrint("\n   ==============================\n");
         DbgPrint("   | Modem Type: NONE(%02X)       |\n", pDevExt->DataInterface);
         DbgPrint("   |============================|\n\n");
         #endif // QCNET_WHQL
         pDevExt->ucModelType = MODELTYPE_NONE;
         gModemType = MODELTYPE_NONE;
      }
   } // if (NT_SUCCESS)

   if (pIntinfo != NULL)
   {
      USBDSP_GetMUXInterface(pDevExt, pIntinfo->InterfaceNumber);
   }

   if (pDevExt->MuxInterface.MuxEnabled == 0x01)
   {
      ntStatus = STATUS_SUCCESS;
      if (pDevExt->MuxInterface.PhysicalInterfaceNumber != pDevExt->MuxInterface.InterfaceNumber)
      {
         pDevExt->ucModelType = MODELTYPE_NET_LIKE;
      }
   }
   
   if (NT_SUCCESS(ntStatus) && pDevExt->ucModelType == MODELTYPE_NONE)
   {
      #ifndef QCNET_WHQL
      DbgPrint("<%s> failed to identify device type!\n", gDeviceName);
      #endif  // QCNET_WHQL
      ntStatus = STATUS_UNSUCCESSFUL;
   }
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
}  // USBPNP_SelectInterfaces

NTSTATUS USBPNP_HandleRemoveDevice
(
   IN PDEVICE_OBJECT DeviceObject,
   IN PDEVICE_OBJECT CalledDO,
   IN PIRP Irp
)
{
   KEVENT ePNPEvent;
   PDEVICE_EXTENSION pDevExt;
   NTSTATUS ntStatus = STATUS_SUCCESS;
   #ifdef QCUSB_TARGET_XP
   KLOCK_QUEUE_HANDLE levelOrHandle;
   #else
   KIRQL levelOrHandle;
   #endif

   pDevExt = DeviceObject->DeviceExtension;

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> USBPNP_HandleRemoveDevice: FDO 0x%p\n", pDevExt->PortName, DeviceObject)
   );
   KeInitializeEvent(&ePNPEvent, SynchronizationEvent, FALSE);

   // Send the IRP down the stack FIRST, to finish the removal

   IoCopyCurrentIrpStackLocationToNext(Irp);

   IoSetCompletionRoutine
   (
      Irp,
      USBMAIN_IrpCompletionSetEvent,
      &ePNPEvent,
      TRUE,TRUE,TRUE
   );

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> USBPNP_HandleRemoveDevice: IoCallDriver 0x%p\n", pDevExt->PortName, pDevExt->StackDeviceObject)
   );
   KeClearEvent(&ePNPEvent);
   ntStatus = IoCallDriver
              (
                 pDevExt->StackDeviceObject,
                 Irp
              );
   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> USBPNP_HandleRemoveDevice: IoCallDriver st 0x%x \n", pDevExt->PortName, ntStatus)
   );
   ntStatus = KeWaitForSingleObject
              (
                 &ePNPEvent, Executive, KernelMode, FALSE, NULL
              );
   KeClearEvent(&ePNPEvent);

   IoDetachDevice(pDevExt->StackDeviceObject);

   Irp->IoStatus.Status = STATUS_SUCCESS;

   return ntStatus;
} // USBPNP_HandleRemoveDevice

NTSTATUS USBPNP_StopDevice( IN  PDEVICE_OBJECT DeviceObject )
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
}  // USBPNP_StopDdevice

NTSTATUS USBPNP_InitDevExt
(
#ifdef NDIS_WDM
   NDIS_HANDLE     WrapperConfigurationContext,
   LONG            QosSetting,
#else
   PDRIVER_OBJECT  pDriverObject,
   LONG            Reserved,
#endif
   PDEVICE_OBJECT  PhysicalDeviceObject,
   PDEVICE_OBJECT  deviceObject,
   char*           myPortName
)
{
   NTSTATUS            ntStatus               = STATUS_SUCCESS;
   PDEVICE_EXTENSION   pDevExt                = NULL;
   PUCHAR              pucNewReadBuffer       = NULL;
   char                myDeviceName[32];

   UNICODE_STRING ucDeviceMapEntry;  // "\Device\QCOMSERn(nn)"
   UNICODE_STRING ucPortName;        // "COMn(n)"
   UNICODE_STRING ucDeviceNumber;    // "n(nn)"
   UNICODE_STRING ucDeviceNameBase;  // "QCOMSER" from registry
   UNICODE_STRING ucDeviceName;      // "QCOMSERn(nn)"
   UNICODE_STRING ucValueName;
   UNICODE_STRING tmpUnicodeString;
   ANSI_STRING    tmpAnsiString;
   POWER_STATE    initialPwrState;
   int            i;
   ULONG selectiveSuspendIdleTime = 0;
   ULONG selectiveSuspendInMili = 0;

   _zeroUnicode(ucDeviceMapEntry);
   _zeroUnicode(ucPortName);
   _zeroUnicode(ucDeviceNumber);
   _zeroUnicode(ucDeviceNameBase);
   _zeroUnicode(ucDeviceName);
   _zeroUnicode(tmpUnicodeString);

   QCUSB_DbgPrintG
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> USBPNP_InitDevExt: 0x%p\n", myPortName, deviceObject)
   );
   // Initialize device extension
   pDevExt = deviceObject->DeviceExtension;
   RtlZeroMemory(pDevExt, sizeof(DEVICE_EXTENSION));

   // StackDeviceObject will be assigned by FDO
   pDevExt->PhysicalDeviceObject = PhysicalDeviceObject;

   strcpy(pDevExt->PortName, myPortName);
   pDevExt->bFdoReused           = FALSE;
   pDevExt->bVendorFeature       = FALSE;
   pDevExt->MyDeviceObject       = deviceObject;

   // device config parameters
   pDevExt->UseMultiReads       = gVendorConfig.UseMultiReads;
   pDevExt->UseMultiWrites      = gVendorConfig.UseMultiWrites;
   pDevExt->ContinueOnOverflow  = gVendorConfig.ContinueOnOverflow;
   pDevExt->ContinueOnDataError = gVendorConfig.ContinueOnDataError;
   pDevExt->EnableLogging       = gVendorConfig.EnableLogging;
   pDevExt->NoTimeoutOnCtlReq   = gVendorConfig.NoTimeoutOnCtlReq;
   pDevExt->MinInPktSize        = gVendorConfig.MinInPktSize;
   pDevExt->NumOfRetriesOnError = gVendorConfig.NumOfRetriesOnError;
   pDevExt->NumberOfL2Buffers   = gVendorConfig.NumberOfL2Buffers;
   pDevExt->DebugMask           = gVendorConfig.DebugMask;
   pDevExt->DebugLevel          = gVendorConfig.DebugLevel;
   pDevExt->MaxPipeXferSize     = gVendorConfig.MaxPipeXferSize;
#ifdef NDIS_WDM
   pDevExt->QosSetting          = QosSetting;
#endif // NDIS_WDM

   // allocate the read buffer here, before we decide to create the PTDO
   pDevExt->TlpFrameBuffer.Buffer = ExAllocatePool(NonPagedPool, QCUSB_RECEIVE_BUFFER_SIZE);
   if (pDevExt->TlpFrameBuffer.Buffer == NULL)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_FORCE,
         ("<%s> USBPNP_InitDevExt: NO MEM - A\n", myPortName)
      );
      ntStatus = STATUS_NO_MEMORY;
      goto USBPNP_InitDevExt_Return;
   }
   pDevExt->TlpFrameBuffer.PktLength   = 0;
   pDevExt->TlpFrameBuffer.BytesNeeded = 0;

   pucNewReadBuffer =  _ExAllocatePool
                       (
                          NonPagedPool,
                          2 + pDevExt->lReadBufferSize,
                          "pucNewReadBuffer"
                       );
   if (!pucNewReadBuffer)
   {
      pDevExt->lReadBufferSize /= 2;  // reduce the size and try again
      pucNewReadBuffer =  _ExAllocatePool
                          (
                             NonPagedPool,
                             2 + pDevExt->lReadBufferSize,
                             "pucNewReadBuffer"
                          );
      if (!pucNewReadBuffer)
      {
         QCUSB_DbgPrint
         (
            QCUSB_DBG_MASK_CONTROL,
            QCUSB_DBG_LEVEL_FORCE,
            ("<%s> USBPNP_InitDevExt: NO MEM - 0\n", myPortName)
         );
         ntStatus = STATUS_NO_MEMORY;
         goto USBPNP_InitDevExt_Return;
      }
   }

   // Initialize L2 Buffers
   // ntStatus = USBRD_InitializeL2Buffers(pDevExt);
   // if (!NT_SUCCESS(ntStatus))
   // {
   //   QCUSB_DbgPrint
   //   (
   //      QCUSB_DBG_MASK_CONTROL,
   //      QCUSB_DBG_LEVEL_FORCE,
   //      ("<%s> USBPNP_InitDevExt: L2 NO MEM - 7a\n", myPortName)
   //   );
   //   goto USBPNP_InitDevExt_Return;
   // }

   #ifdef QCUSB_MULTI_WRITES
   ntStatus = USBMWT_InitializeMultiWriteElements(pDevExt);
   if (!NT_SUCCESS(ntStatus))
   {
     QCUSB_DbgPrintG
     (
        QCUSB_DBG_MASK_CONTROL,
        QCUSB_DBG_LEVEL_FORCE,
        ("<%s> USBPNP: MWT NO MEM\n", myPortName)
     );
     goto USBPNP_InitDevExt_Return;
   }
   #endif // QCUSB_MULTI_WRITES

   pDevExt->bInService     = FALSE;  //open the gate
   pDevExt->bL1ReadActive  = FALSE;
   pDevExt->bWriteActive   = FALSE;

   pDevExt->bReadBufferReset = FALSE;

   // initialize our unicode strings for object deletion

   pDevExt->ucsDeviceMapEntry.Buffer = ucDeviceMapEntry.Buffer;
   pDevExt->ucsDeviceMapEntry.Length = ucDeviceMapEntry.Length;
   pDevExt->ucsDeviceMapEntry.MaximumLength =
      ucDeviceMapEntry.MaximumLength;
   ucDeviceMapEntry.Buffer = NULL; // we've handed off that buffer

   pDevExt->ucsPortName.Buffer = ucPortName.Buffer;
   pDevExt->ucsPortName.Length = ucPortName.Length;
   pDevExt->ucsPortName.MaximumLength = ucPortName.MaximumLength;
   ucPortName.Buffer = NULL;

   pDevExt->pucReadBufferStart = pucNewReadBuffer;
   pucNewReadBuffer            = NULL; // so we won't free it on exit from this sbr

   // setup flow limits
   pDevExt->lReadBufferLow   = pDevExt->lReadBufferSize / 10;       /* 10% */
   pDevExt->lReadBufferHigh  = (pDevExt->lReadBufferSize * 9) / 10; /* 90% */
   pDevExt->lReadBuffer80pct = (pDevExt->lReadBufferSize * 4) / 5;  /* 80% */
   pDevExt->lReadBuffer50pct = pDevExt->lReadBufferSize / 2;        /* 50% */
   pDevExt->lReadBuffer20pct = pDevExt->lReadBufferSize / 5;        /* 20% */

   pDevExt->pNotificationIrp   = NULL;

   // Init locks, events, and mutex
   _InitializeMutex(&pDevExt->muPnPMutex);
   KeInitializeSpinLock(&pDevExt->ControlSpinLock);
   KeInitializeSpinLock(&pDevExt->ReadSpinLock);
   KeInitializeSpinLock(&pDevExt->WriteSpinLock);
   KeInitializeSpinLock(&pDevExt->SingleIrpSpinLock);

   KeInitializeEvent
   (
      &pDevExt->DSPSyncEvent,
      SynchronizationEvent,
      TRUE
   );
   KeSetEvent(&pDevExt->DSPSyncEvent,IO_NO_INCREMENT,FALSE);
   KeInitializeEvent
   (
      &pDevExt->ForTimeoutEvent,
      SynchronizationEvent,
      TRUE
   );
   KeInitializeEvent
   (
      &pDevExt->IntThreadStartedEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->InterruptPipeClosedEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->ReadThreadStartedEvent,
      SynchronizationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->L2ReadThreadStartedEvent,
      SynchronizationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->WriteThreadStartedEvent,
      SynchronizationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->DspThreadStartedEvent,
      SynchronizationEvent,
      FALSE
   );

   // interrupt pipe thread waits on these
   pDevExt->pInterruptPipeEvents[INT_COMPLETION_EVENT_INDEX] =
      &pDevExt->eInterruptCompletion;
   KeInitializeEvent(&pDevExt->eInterruptCompletion,NotificationEvent,FALSE);
   pDevExt->pInterruptPipeEvents[CANCEL_EVENT_INDEX] =
      &pDevExt->CancelInterruptPipeEvent;
   KeInitializeEvent
   (
      &pDevExt->CancelInterruptPipeEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pInterruptPipeEvents[INT_STOP_SERVICE_EVENT] =
      &pDevExt->InterruptStopServiceEvent;
   KeInitializeEvent
   (
      &pDevExt->InterruptStopServiceEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->InterruptStopServiceRspEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pInterruptPipeEvents[INT_RESUME_SERVICE_EVENT] =
      &pDevExt->InterruptResumeServiceEvent;
   KeInitializeEvent
   (
      &pDevExt->InterruptResumeServiceEvent,
      NotificationEvent,
      FALSE
   );

   pDevExt->pInterruptPipeEvents[INT_EMPTY_RD_QUEUE_EVENT_INDEX] =
      &pDevExt->InterruptEmptyRdQueueEvent;
   pDevExt->pInterruptPipeEvents[INT_EMPTY_WT_QUEUE_EVENT_INDEX] =
      &pDevExt->InterruptEmptyWtQueueEvent;
   pDevExt->pInterruptPipeEvents[INT_EMPTY_CTL_QUEUE_EVENT_INDEX] =
      &pDevExt->InterruptEmptyCtlQueueEvent;
   pDevExt->pInterruptPipeEvents[INT_EMPTY_SGL_QUEUE_EVENT_INDEX] =
      &pDevExt->InterruptEmptySglQueueEvent;
   pDevExt->pInterruptPipeEvents[INT_REG_IDLE_NOTIF_EVENT_INDEX] =
      &pDevExt->InterruptRegIdleEvent;
   pDevExt->pInterruptPipeEvents[INT_IDLE_EVENT] = &pDevExt->InterruptIdleEvent;
   KeInitializeEvent
   (
      &pDevExt->InterruptIdleEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pInterruptPipeEvents[INT_IDLENESS_COMPLETED_EVENT] = &pDevExt->InterruptIdlenessCompletedEvent;
   KeInitializeEvent
   (
      &pDevExt->InterruptIdlenessCompletedEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pInterruptPipeEvents[INT_IDLE_CBK_COMPLETED_EVENT] = &pDevExt->IdleCbkCompleteEvent;
   KeInitializeEvent
   (
      &pDevExt->IdleCbkCompleteEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->IdleCallbackInProgress = 0;
   pDevExt->bIdleIrpCompleted      = FALSE;
   pDevExt->bLowPowerMode          = FALSE;

   KeInitializeEvent
   (
      &pDevExt->InterruptEmptyRdQueueEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->InterruptEmptyWtQueueEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->InterruptEmptyCtlQueueEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->InterruptEmptySglQueueEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->InterruptRegIdleEvent,
      NotificationEvent,
      FALSE
   );

   // write thread waits on these
   pDevExt->pWriteEvents[CANCEL_EVENT_INDEX] = &pDevExt->CancelWriteEvent;
   KeInitializeEvent(&pDevExt->CancelWriteEvent,NotificationEvent,FALSE);
   pDevExt->pWriteEvents[WRITE_COMPLETION_EVENT_INDEX] = &pDevExt->WriteCompletionEvent;
   KeInitializeEvent
   (
      &pDevExt->WriteCompletionEvent,
      NotificationEvent,
      FALSE
   );

   pDevExt->pWriteEvents[KICK_WRITE_EVENT_INDEX] = &pDevExt->KickWriteEvent;
   KeInitializeEvent(&pDevExt->KickWriteEvent, NotificationEvent, FALSE);

   pDevExt->pWriteEvents[WRITE_CANCEL_CURRENT_EVENT_INDEX] =
      &pDevExt->WriteCancelCurrentEvent;
   KeInitializeEvent(&pDevExt->WriteCancelCurrentEvent, NotificationEvent, FALSE);

   pDevExt->pWriteEvents[WRITE_PURGE_EVENT_INDEX] = &pDevExt->WritePurgeEvent;
   KeInitializeEvent(&pDevExt->WritePurgeEvent, NotificationEvent, FALSE);

   pDevExt->pWriteEvents[WRITE_STOP_EVENT_INDEX] = &pDevExt->WriteStopEvent;
   KeInitializeEvent(&pDevExt->WriteStopEvent, NotificationEvent, FALSE);

   pDevExt->pWriteEvents[WRITE_RESUME_EVENT_INDEX] = &pDevExt->WriteResumeEvent;
   KeInitializeEvent(&pDevExt->WriteResumeEvent, NotificationEvent, FALSE);

   pDevExt->pWriteEvents[WRITE_FLOW_ON_EVENT_INDEX] = &pDevExt->WriteFlowOnEvent;
   KeInitializeEvent(&pDevExt->WriteFlowOnEvent, NotificationEvent, FALSE);

   pDevExt->pWriteEvents[WRITE_FLOW_OFF_EVENT_INDEX] = &pDevExt->WriteFlowOffEvent;
   KeInitializeEvent(&pDevExt->WriteFlowOffEvent, NotificationEvent, FALSE);
   KeInitializeEvent(&pDevExt->WriteFlowOffAckEvent, NotificationEvent, FALSE);

   // read thread waits on these
   pDevExt->pL2ReadEvents[CANCEL_EVENT_INDEX] = &pDevExt->CancelReadEvent;
   KeInitializeEvent(&pDevExt->CancelReadEvent,NotificationEvent,FALSE);

   pDevExt->pL1ReadEvents[L1_CANCEL_EVENT_INDEX] = &pDevExt->L1CancelReadEvent;
   KeInitializeEvent(&pDevExt->L1CancelReadEvent,NotificationEvent,FALSE);

   pDevExt->pL2ReadEvents[L2_READ_COMPLETION_EVENT_INDEX] = &pDevExt->L2ReadCompletionEvent;
   pDevExt->pL1ReadEvents[L1_READ_COMPLETION_EVENT_INDEX] = &pDevExt->L1ReadCompletionEvent;
   pDevExt->pL1ReadEvents[L1_READ_AVAILABLE_EVENT_INDEX] = &pDevExt->L1ReadAvailableEvent;

   KeInitializeEvent(&pDevExt->L2ReadCompletionEvent, NotificationEvent, FALSE);
   KeInitializeEvent(&pDevExt->L1ReadCompletionEvent, NotificationEvent, FALSE);
   KeInitializeEvent(&pDevExt->L1ReadAvailableEvent, NotificationEvent, FALSE);
   KeInitializeEvent(&pDevExt->L1ReadPurgeAckEvent, NotificationEvent, FALSE);

   pDevExt->pL2ReadEvents[L2_KICK_READ_EVENT_INDEX] = &pDevExt->L2KickReadEvent;
   KeInitializeEvent(&pDevExt->L2KickReadEvent, NotificationEvent, FALSE);

   pDevExt->pL2ReadEvents[L2_STOP_EVENT_INDEX] = &pDevExt->L2StopEvent;
   KeInitializeEvent(&pDevExt->L2StopEvent, NotificationEvent, FALSE);
   KeInitializeEvent(&pDevExt->L2StopAckEvent, NotificationEvent, FALSE);

   pDevExt->pL2ReadEvents[L2_RESUME_EVENT_INDEX] = &pDevExt->L2ResumeEvent;
   pDevExt->pL2ReadEvents[L2_USB_READ_EVENT_INDEX] = &pDevExt->L2USBReadCompEvent;
   
   KeInitializeEvent(&pDevExt->L2ResumeEvent, NotificationEvent, FALSE);
   KeInitializeEvent(&pDevExt->L2USBReadCompEvent, NotificationEvent, FALSE);
   
   KeInitializeEvent(&pDevExt->L2ResumeAckEvent, NotificationEvent, FALSE);

   pDevExt->pL1ReadEvents[L1_KICK_READ_EVENT_INDEX] = &pDevExt->L1KickReadEvent;
   KeInitializeEvent(&pDevExt->L1KickReadEvent, NotificationEvent, FALSE);

   pDevExt->pL1ReadEvents[L1_READ_PURGE_EVENT_INDEX] = &pDevExt->L1ReadPurgeEvent;
   KeInitializeEvent(&pDevExt->L1ReadPurgeEvent, NotificationEvent, FALSE);

   pDevExt->pL1ReadEvents[L1_STOP_EVENT_INDEX] = &pDevExt->L1StopEvent;
   KeInitializeEvent(&pDevExt->L1StopEvent, NotificationEvent, FALSE);
   KeInitializeEvent(&pDevExt->L1StopAckEvent, NotificationEvent, FALSE);

   pDevExt->pL1ReadEvents[L1_RESUME_EVENT_INDEX] = &pDevExt->L1ResumeEvent;
   KeInitializeEvent(&pDevExt->L1ResumeEvent, NotificationEvent, FALSE);
   KeInitializeEvent(&pDevExt->L1ResumeAckEvent, NotificationEvent, FALSE);

   KeInitializeEvent
   (
      &pDevExt->DispatchReadControlEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->L1ReadThreadClosedEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->L2ReadThreadClosedEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->ReadIrpPurgedEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->WriteThreadClosedEvent,
      NotificationEvent,
      FALSE
   );

   pDevExt->pDispatchEvents[DSP_CANCEL_DISPATCH_EVENT_INDEX] = &pDevExt->DispatchCancelEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchCancelEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pDispatchEvents[DSP_PURGE_EVENT_INDEX] = &pDevExt->DispatchPurgeEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchPurgeEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pDispatchEvents[DSP_START_POLLING_EVENT_INDEX] = &pDevExt->DispatchStartPollingEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchStartPollingEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pDispatchEvents[DSP_STOP_POLLING_EVENT_INDEX] = &pDevExt->DispatchStopPollingEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchStopPollingEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pDispatchEvents[DSP_STANDBY_EVENT_INDEX] = &pDevExt->DispatchStandbyEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchStandbyEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pDispatchEvents[DSP_WAKEUP_EVENT_INDEX] = &pDevExt->DispatchWakeupEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchWakeupEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pDispatchEvents[DSP_WAKEUP_RESET_EVENT_INDEX] = &pDevExt->DispatchWakeupResetEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchWakeupResetEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->DispatchPurgeCompleteEvent,
      NotificationEvent,
      FALSE
   );

   pDevExt->pDispatchEvents[DSP_START_DISPATCH_EVENT_INDEX] = &pDevExt->DispatchStartEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchStartEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->DispatchThreadClosedEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pDispatchEvents[DSP_PRE_WAKEUP_EVENT_INDEX] = &pDevExt->DispatchPreWakeupEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchPreWakeupEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pDispatchEvents[DSP_XWDM_OPEN_EVENT_INDEX] = &pDevExt->DispatchXwdmOpenEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchXwdmOpenEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pDispatchEvents[DSP_XWDM_CLOSE_EVENT_INDEX] = &pDevExt->DispatchXwdmCloseEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchXwdmCloseEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pDispatchEvents[DSP_XWDM_QUERY_RM_EVENT_INDEX] = &pDevExt->DispatchXwdmQueryRmEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchXwdmQueryRmEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pDispatchEvents[DSP_XWDM_RM_CANCELLED_EVENT_INDEX] = &pDevExt->DispatchXwdmReopenEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchXwdmReopenEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pDispatchEvents[DSP_XWDM_QUERY_S_PWR_EVENT_INDEX] = &pDevExt->DispatchXwdmQuerySysPwrEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchXwdmQuerySysPwrEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeEvent
   (
      &pDevExt->DispatchXwdmQuerySysPwrAckEvent,
      NotificationEvent,
      FALSE
   );
   pDevExt->pDispatchEvents[DSP_XWDM_NOTIFY_EVENT_INDEX] = &pDevExt->DispatchXwdmNotifyEvent;
   KeInitializeEvent
   (
      &pDevExt->DispatchXwdmNotifyEvent,
      NotificationEvent,
      FALSE
   );
   KeInitializeSpinLock(&pDevExt->ExWdmSpinLock);
   pDevExt->ExWdmInService = FALSE;

   // Initialize device extension
   InitializeListHead(&pDevExt->RdCompletionQueue);
   InitializeListHead(&pDevExt->WtCompletionQueue);
   InitializeListHead(&pDevExt->CtlCompletionQueue);
   InitializeListHead(&pDevExt->SglCompletionQueue);
   InitializeListHead(&pDevExt->EncapsulatedDataQueue);
   InitializeListHead(&pDevExt->DispatchQueue);
   InitializeListHead(&pDevExt->ReadDataQueue);
   InitializeListHead(&pDevExt->WriteDataQueue);
   InitializeListHead(&pDevExt->MWTSentIrpQueue);
   InitializeListHead(&pDevExt->MWTSentIrpRecordPool);
   InitializeListHead(&pDevExt->IdleIrpCompletionStack);

   pDevExt->hInterruptThreadHandle = NULL;
   pDevExt->hL1ReadThreadHandle    = NULL;
   pDevExt->hL2ReadThreadHandle    = NULL;
   pDevExt->hWriteThreadHandle     = NULL;
   pDevExt->hTxLogFile             = NULL;
   pDevExt->hRxLogFile             = NULL;
   pDevExt->bL1InCancellation      = FALSE;
   pDevExt->bEnableResourceSharing = FALSE;
   pDevExt->pReadControlEvent  = &(pDevExt->DispatchReadControlEvent);
   pDevExt->pRspAvailableCount = &(pDevExt->lRspAvailableCount);

   KeClearEvent(&pDevExt->L1ReadThreadClosedEvent);
   KeClearEvent(&pDevExt->L2ReadThreadClosedEvent);
   KeClearEvent(&pDevExt->ReadIrpPurgedEvent);
   KeClearEvent(&pDevExt->L1ReadPurgeAckEvent);
   KeClearEvent(&pDevExt->WriteThreadClosedEvent);
   KeClearEvent(&pDevExt->InterruptPipeClosedEvent);
   KeClearEvent(&pDevExt->InterruptStopServiceEvent);
   KeClearEvent(&pDevExt->InterruptStopServiceRspEvent);
   KeClearEvent(&pDevExt->InterruptResumeServiceEvent);
   KeClearEvent(&pDevExt->CancelReadEvent);
   KeClearEvent(&pDevExt->L1CancelReadEvent);
   KeClearEvent(&pDevExt->CancelWriteEvent);
   // KeClearEvent(&pDevExt->CancelInterruptPipeEvent);

   initDevState(DEVICE_STATE_ZERO);
   pDevExt->bDeviceRemoved = FALSE;
   pDevExt->bDeviceSurpriseRemoved = FALSE;
   pDevExt->pRemoveLock = &pDevExt->RemoveLock;
   pDevExt->PowerState = PowerDeviceUnspecified;
   pDevExt->SetCommFeatureSupported = FALSE;
   IoInitializeRemoveLock(pDevExt->pRemoveLock, 0, 0, 0);

   //Initialize all thread synchronization
   pDevExt->DispatchThreadCancelStarted = 0;
   pDevExt->InterruptThreadCancelStarted = 0;
   pDevExt->ReadThreadCancelStarted = 0;
   pDevExt->ReadThreadInCreation = 0;
   pDevExt->WriteThreadCancelStarted = 0;
   pDevExt->WriteThreadInCreation = 0;
   

   #ifdef QCUSB_PWR
   KeInitializeEvent
   (
      &pDevExt->RegIdleAckEvent,
      NotificationEvent,
      FALSE
   );

   pDevExt->PMWmiRegistered   = FALSE;
   pDevExt->PowerSuspended    = FALSE;
   pDevExt->SelectiveSuspended = FALSE;
   pDevExt->PrepareToPowerDown= FALSE;
   pDevExt->IdleTimerLaunched = FALSE;
   pDevExt->IoBusyMask        = 0;
   pDevExt->SelectiveSuspendIdleTime = 5;  // chage it to 5 by default
   pDevExt->SelectiveSuspendInMili = FALSE;   
   pDevExt->InServiceSelectiveSuspension = TRUE;   
   pDevExt->bRemoteWakeupEnabled = FALSE;  // hardcode for now
   pDevExt->bDeviceSelfPowered = FALSE;
   pDevExt->PowerManagementEnabled = TRUE;
   pDevExt->WaitWakeEnabled = FALSE;  // hardcode for now
   QCPWR_GetWdmVersion(pDevExt);
   KeInitializeTimer(&pDevExt->IdleTimer);
   KeInitializeDpc(&pDevExt->IdleDpc, QCPWR_IdleDpcRoutine, pDevExt);
   pDevExt->SystemPower = PowerSystemWorking;
   pDevExt->DevicePower = initialPwrState.DeviceState = PowerDeviceD0;
   // PoSetPowerState(fdo, DevicePowerState, initialPwrState);
   USBPNP_GetDeviceCapabilities(pDevExt, bPowerManagement); // store info in portExt
   pDevExt->bPowerManagement = bPowerManagement;
   InitializeListHead(&pDevExt->OriginatedPwrReqQueue);
   InitializeListHead(&pDevExt->OriginatedPwrReqPool);
   for (i = 0; i < SELF_ORIGINATED_PWR_REQ_MAX; i++)
   {
      InsertTailList
      (
         &pDevExt->OriginatedPwrReqPool,
         &(pDevExt->SelfPwrReq[i].List)
      );
   }

   #endif 

USBPNP_InitDevExt_Return:
   if (pucNewReadBuffer)
   {
      ExFreePool(pucNewReadBuffer);
      pucNewReadBuffer = NULL;
   }

   _freeString(ucDeviceMapEntry);
   _freeString(ucPortName);
   _freeString(ucDeviceNumber);
   _freeString(ucDeviceNameBase);
   _freeString(ucDeviceName);

   if (NT_SUCCESS(ntStatus))
   {
#ifndef NDIS_WDM
      ntStatus = QCUSB_PostVendorRegistryProcess
                 (
                    pDriverObject,
                    PhysicalDeviceObject,
                    deviceObject
                 );
#endif // NDIS_WDM

      // Get the SS value here
      {
         ANSI_STRING ansiStr;
         UNICODE_STRING unicodeStr;
         RtlInitAnsiString( &ansiStr, gServiceName);
         RtlAnsiStringToUnicodeString(&unicodeStr, &ansiStr, TRUE);
         selectiveSuspendIdleTime = 0;
         ntStatus = USBUTL_GetServiceRegValue( unicodeStr.Buffer, VEN_DRV_SS_IDLE_T, &selectiveSuspendIdleTime);
         RtlFreeUnicodeString( &unicodeStr );
      }
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> USBUTL_GetServiceRegValue: returned 0x%x Value 0x%x\n", myPortName, ntStatus, selectiveSuspendIdleTime)
      );
      
      selectiveSuspendInMili = selectiveSuspendIdleTime & 0x40000000;
      
      selectiveSuspendIdleTime &= 0x00FFFFFF;
      
      if (selectiveSuspendInMili == 0)
      {
          pDevExt->SelectiveSuspendInMili = FALSE;
          if (selectiveSuspendIdleTime < QCUSB_SS_IDLE_MIN)
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
          pDevExt->SelectiveSuspendInMili = TRUE;
          if (selectiveSuspendIdleTime < QCUSB_SS_MILI_IDLE_MIN)
          {
             selectiveSuspendIdleTime = QCUSB_SS_MILI_IDLE_MIN;
          }
          else if (selectiveSuspendIdleTime > QCUSB_SS_MILI_IDLE_MAX)
          {
             selectiveSuspendIdleTime = QCUSB_SS_MILI_IDLE_MAX;
          }
          
      }
      pDevExt->SelectiveSuspendIdleTime = selectiveSuspendIdleTime;

      if (!NT_SUCCESS(ntStatus))
      {
         ntStatus = STATUS_SUCCESS;
      }
   }

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> USBPNP_InitDevExt: exit 0x%x\n", myPortName, ntStatus)
   );
   return ntStatus;
}  // USBPNP_InitDevExt

BOOLEAN USBPNP_ValidateConfigDescriptor
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
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _ValidateConfigDescriptor: bad length: %uB, %uB\n",
           pDevExt->PortName, ConfigDesc->bLength, ConfigDesc->wTotalLength
         )
      );
      return FALSE;
   }

   if (ConfigDesc->bNumInterfaces >= MAX_INTERFACE)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _ValidateConfigDescriptor: bad bNumInterfaces 0x%x\n",
           pDevExt->PortName, ConfigDesc->bNumInterfaces)
      );
      return FALSE;
   }

   if (ConfigDesc->bDescriptorType != 0x02)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _ValidateConfigDescriptor: bad bDescriptorType 0x%x\n",
           pDevExt->PortName, ConfigDesc->bDescriptorType)
      );
      return FALSE;
   }

#if 0
   if (ConfigDesc->bConfigurationValue != 1)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _ValidateConfigDescriptor: bad bConfigurationValue 0x%x\n",
           pDevExt->PortName, ConfigDesc->bConfigurationValue)
      );
      return FALSE;
   }
#endif
   return TRUE;

}  // USBPNP_ValidateConfigDescriptor

BOOLEAN USBPNP_ValidateDeviceDescriptor
(
   PDEVICE_EXTENSION      pDevExt,
   PUSB_DEVICE_DESCRIPTOR DevDesc
)
{
   if (DevDesc->bLength == 0)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _ValidateDeviceDescriptor: 0 bLength\n", pDevExt->PortName)
      );
      return FALSE;
   }

   if (DevDesc->bDescriptorType != 0x01)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
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
         QCUSB_DbgPrint
         (
            QCUSB_DBG_MASK_CONTROL,
            QCUSB_DBG_LEVEL_ERROR,
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
         QCUSB_DbgPrint
         (
            QCUSB_DBG_MASK_CONTROL,
            QCUSB_DBG_LEVEL_ERROR,
            ("<%s> _ValidateDeviceDescriptor: bad SS bMaxPacketSize0 0x%x\n",
              pDevExt->PortName, DevDesc->bMaxPacketSize0)
         );
         return FALSE;
      }
   }

#if 0
   if (DevDesc->bNumConfigurations != 0x01)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _ValidateDeviceDescriptor: bad bNumConfigurations 0x%x\n",
           pDevExt->PortName, DevDesc->bNumConfigurations)
      );
      return FALSE;
   }
#endif
   return TRUE;

}  // USBPNP_ValidateDeviceDescriptor

