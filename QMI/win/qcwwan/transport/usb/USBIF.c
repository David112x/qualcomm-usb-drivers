/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             U S B I F . C

GENERAL DESCRIPTION
  This file contains USB component entry points

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#include "USBMAIN.h"
#include "USBIF.h"
#include "USBPNP.h"
#include "USBDSP.h"
#include "USBRD.h"
#include "USBWT.h"
#include "USBUTL.h"
#include "USBDSP.h"
#include "USBCTL.h"
#include "USBINT.h"
#include "USBPWR.h"
#include "MPMAIN.h"
#include "MPQMUX.h"

// EVENT_TRACING
#ifdef EVENT_TRACING

#include "MPWPP.h"               // Driver specific WPP Macros, Globals, etc 
#include "USBIF.tmh"

#endif   // EVENT_TRACING

extern LONG GetQMUXTransactionId(PMP_ADAPTER pAdapter);

extern PMP_ADAPTER FindStackDeviceObject(PMP_ADAPTER pAdapter);

PDRIVER_DISPATCH QCDriverDispatchTable[IRP_MJ_MAXIMUM_FUNCTION + 1];
extern NTKERNELAPI VOID IoReuseIrp(IN OUT PIRP Irp, IN NTSTATUS Iostatus);
extern GUID gQcFeatureGUID;
#ifdef DEBUG_MSGS

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
#endif // DEBUG_MSGS

#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif // QCUSB_MUX_PROTOCOL

PDEVICE_OBJECT USBIF_InitializeUSB
(
   IN PVOID          MiniportContext,
   IN PDRIVER_OBJECT DriverObject,
   IN PDEVICE_OBJECT Pdo,
   IN PDEVICE_OBJECT Fdo,
   IN PDEVICE_OBJECT NextDO,
   IN NDIS_HANDLE    WrapperConfigurationContext,
   PCHAR             PortName
)
{
   PDEVICE_OBJECT pUsbDO;
   PDEVICE_EXTENSION pDevExt;
   PQCUSB_ENTRY_POINTS usbEntries;
   int i;
   NTSTATUS ntStatus;
   PMP_ADAPTER pAdapter = (PMP_ADAPTER)MiniportContext;

   if (PortName == NULL)
   {
      strcpy(gDeviceName, "qcusbnet");
   }
   else
   {
      strcpy(gDeviceName, PortName);
   }

   pUsbDO = USBIF_CreateUSBComponent
            (
               MiniportContext,
               WrapperConfigurationContext,
               Pdo,
               Fdo,
               NextDO,
               pAdapter->MPDisableQoS
            );

   if (pUsbDO != NULL)
   {
      pDevExt = (PDEVICE_EXTENSION)pUsbDO->DeviceExtension;
      pDevExt->MyDriverObject = DriverObject;
      pDevExt->MiniportContext = MiniportContext;
      usbEntries = &pDevExt->EntryPoints;
      for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
      {
         usbEntries->MajorFunction[i] = USBDSP_DirectDispatch;
      }
      usbEntries->MajorFunction[IRP_MJ_WRITE]          = USBWT_Write;
      usbEntries->MajorFunction[IRP_MJ_READ]           = USBRD_Read;
      usbEntries->MajorFunction[IRP_MJ_DEVICE_CONTROL] = USBDSP_QueuedDispatch;
      usbEntries->MajorFunction[IRP_MJ_POWER]          = USBDSP_QueuedDispatch;
      usbEntries->MajorFunction[IRP_MJ_PNP]            = USBDSP_QueuedDispatch;
      usbEntries->DriverUnload = USBMAIN_Unload;

      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> USBIF_InitializeUSB: StartDevice...\n", PortName)
      );
      #ifdef NDIS_WDM
      pDevExt->NdisConfigurationContext = WrapperConfigurationContext;
      #endif
      ntStatus = USBPNP_StartDevice(pUsbDO, 1);
      if (!NT_SUCCESS(ntStatus))
      {
         USBRD_CancelReadThread(pDevExt, 4);
         USBWT_CancelWriteThread(pDevExt, 4);
         USBDSP_CancelDispatchThread(pDevExt, 3);
         USBINT_CancelInterruptThread(pDevExt, 2);
         QCUSB_CleanupDeviceExtensionBuffers(pUsbDO);
         QcIoDeleteDevice(pUsbDO);
         pUsbDO = NULL;
      }
      else
      {
         pDevExt->bDeviceRemoved = FALSE;
         pDevExt->bDeviceSurpriseRemoved = FALSE;
         setDevState(DEVICE_STATE_PRESENT_AND_STARTED);
         ntStatus = USBIF_Open(pUsbDO);
         #ifdef QCUSB_PWR
         if (NT_SUCCESS(ntStatus))
         {
            // USBPWR_StartExWdmDeviceMonitor(pDevExt);
         }
         #endif // QCUSB_PWR
         USBIF_SetDataMTU(pUsbDO, 0); // set default
      }
   }

   return pUsbDO;
}  // USBIF_InitializeUSB

VOID USBIF_ShutdownUSB(PDEVICE_OBJECT DeviceObject)
{
   PDEVICE_EXTENSION pDevExt;

   if (DeviceObject == NULL)
   {
      return;
   }

   pDevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> USBIF_ShutdownUSB\n", pDevExt->PortName)
   );
   USBDSP_CancelDispatchThread(pDevExt, 1);
   QCUSB_CleanupDeviceExtensionBuffers(DeviceObject);
   QcIoDeleteDevice(DeviceObject);
}  // USBIF_ShutdownUSB

NTSTATUS QCIoCallDriver
(
   IN PDEVICE_OBJECT DeviceObject,
   IN OUT PIRP       Irp
)
{
   PDEVICE_EXTENSION pDevExt;
   PQCUSB_ENTRY_POINTS usbEntries;
   PIO_STACK_LOCATION irpStack;

   if (DeviceObject == NULL)
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_FORCE,
         ("<qcnet> ERROR: QcIoCallDriver: NULL DO\n")
      );
      IoSetCancelRoutine(Irp, NULL);
      Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_UNSUCCESSFUL;
   }

   pDevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
   usbEntries = &pDevExt->EntryPoints;

   IoSetNextIrpStackLocation(Irp);
   irpStack = IoGetCurrentIrpStackLocation(Irp);
   irpStack->DeviceObject = DeviceObject;

   if (usbEntries->MajorFunction[irpStack->MajorFunction] != NULL)
   {
      return (*usbEntries->MajorFunction[irpStack->MajorFunction])(DeviceObject, Irp);
   }
   else
   {
      IoSetCancelRoutine(Irp, NULL);
      Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
   }

   return STATUS_NOT_IMPLEMENTED;
}  // QCIoCallDriver

NTSTATUS QCDirectIoCallDriver
(
   IN PDEVICE_OBJECT DeviceObject,
   IN OUT PIRP       Irp
)
{
   PDEVICE_EXTENSION pDevExt;
   PQCUSB_ENTRY_POINTS usbEntries;
   PIO_STACK_LOCATION irpStack;

   if (DeviceObject == NULL)
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_FORCE,
         ("<qcnet> ERROR: QcDirectIoCallDriver: NULL DO\n")
      );
      IoSetCancelRoutine(Irp, NULL);
      Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_UNSUCCESSFUL;
   }

   pDevExt = (PDEVICE_EXTENSION)DeviceObject->DeviceExtension;
   usbEntries = &pDevExt->EntryPoints;

   irpStack = IoGetCurrentIrpStackLocation(Irp);

   if (usbEntries->MajorFunction[irpStack->MajorFunction] != NULL)
   {
      return (*usbEntries->MajorFunction[irpStack->MajorFunction])(DeviceObject, Irp);
   }
   else
   {
      IoSetCancelRoutine(Irp, NULL);
      Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
   }

   return STATUS_NOT_IMPLEMENTED;
}  // QCDirectIoCallDriver

// This function is similar to AddDevice
// 1. Create USB DO
// 2. Init extension and assign function entry points
PDEVICE_OBJECT USBIF_CreateUSBComponent
(
   IN PVOID          MiniportContext,
   IN NDIS_HANDLE    WrapperConfigurationContext,
   IN PDEVICE_OBJECT Pdo,
   IN PDEVICE_OBJECT Fdo,
   IN PDEVICE_OBJECT NextDO,
   IN LONG           QosSetting
)
{
   char                  myPortName[16];
   NTSTATUS              ntStatus  = STATUS_SUCCESS;
   PDEVICE_OBJECT        fdo = NULL;
   PDEVICE_EXTENSION     pDevExt;

   bAddNewDevice = TRUE;
   QcAcquireEntryPass(&gSyncEntryEvent, "qc-add");

   ntStatus = USBIF_NdisVendorRegistryProcess
              (
                 MiniportContext,
                 WrapperConfigurationContext
              );
   if (!NT_SUCCESS(ntStatus))
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> CreateUSBComponent: reg access failure.\n", gDeviceName)
      );
      goto USBIF_CreateUSBComponent_Return;
   }

   QCUSB_DbgPrintG
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> CreateUSBComponent: <%d+%d>\n  PHYDEVO=0x%p\n", gDeviceName,
          sizeof(struct _DEVICE_OBJECT), sizeof(DEVICE_EXTENSION),
          Pdo)
   );

   if (gDeviceName[0] == 0)
   {
      strcpy(myPortName, "qcu");
   }
   else
   {
      strcpy(myPortName, gDeviceName);
   }

   QCUSB_DbgPrintG2
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> CreateUSBComponent: Creating FDO...\n", myPortName)
   );

   /***
   fdo = (PDEVICE_OBJECT)ExAllocatePoolWithTag
         (
            NonPagedPool,
            sizeof(DEVICE_OBJECT),
            (ULONG)'0001'
         );

   if (fdo != NULL)
   {
      fdo->DeviceExtension = ExAllocatePoolWithTag(NonPagedPool, sizeof(DEVICE_EXTENSION), (ULONG)'0002');
      if (fdo->DeviceExtension == NULL)
      {
         ExFreePool(fdo);
         fdo = NULL;
      }
   }
   ***/

   ntStatus = IoCreateDevice
              (
                 Pdo->DriverObject,
                 sizeof(DEVICE_EXTENSION),
                 NULL,                // unnamed
                 FILE_DEVICE_UNKNOWN,
                 0,                   // DeviceCharacteristics
                 FALSE,
                 &fdo
              );
   if (!NT_SUCCESS(ntStatus))
   {
      fdo = NULL;
   }


   if (fdo != NULL)
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> CreateUSBComponent: new FDO 0x%p (stack %d)\n", myPortName, fdo, Fdo->StackSize)
      );
      fdo->StackSize = Fdo->StackSize;
      fdo->Flags |= DO_DIRECT_IO;
      fdo->Flags &= ~DO_EXCLUSIVE;
      fdo->DeviceType = FILE_DEVICE_UNKNOWN;
   }
   else
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_CRITICAL,
         ("<%s> CreateUSBComponent: FDO failure 0x%x\n", myPortName, ntStatus)
      );
      fdo = NULL;
      goto USBIF_CreateUSBComponent_Return;
   }

   pDevExt = (PDEVICE_EXTENSION)fdo->DeviceExtension;

   // initialize device extension
   ntStatus = USBPNP_InitDevExt(WrapperConfigurationContext, QosSetting, Pdo, fdo, myPortName);
   ntStatus = USBIF_NdisPostVendorRegistryProcess
              (
                 MiniportContext,
                 WrapperConfigurationContext,
                 fdo
              );
   if (!NT_SUCCESS(ntStatus))
   {
      QCUSB_CleanupDeviceExtensionBuffers(fdo);
      QcIoDeleteDevice(fdo);
      fdo = NULL;
      goto USBIF_CreateUSBComponent_Return;
   }

   pDevExt->StackDeviceObject = NextDO;
   pDevExt->MiniportFDO = Fdo;

   // Continue initializing extension of the port DO
   pDevExt->bDeviceRemoved = FALSE;
   pDevExt->bDeviceSurpriseRemoved = FALSE;
   pDevExt->bmDevState = DEVICE_STATE_ZERO;

   // USBPNP_GetDeviceCapabilities(pDevExt, bPowerManagement); // store info in portExt
   // pDevExt->bPowerManagement = bPowerManagement;

   fdo->Flags |= DO_POWER_PAGABLE;
   fdo->Flags &= ~DO_DEVICE_INITIALIZING;

   ntStatus = USBDSP_InitDispatchThread(fdo);

   USBIF_CreateUSBComponent_Return:

   QcReleaseEntryPass(&gSyncEntryEvent, "qc-add", "END");
   bAddNewDevice = FALSE;

   return fdo;

}  // USBIF_CreateUSBComponent

NTSTATUS USBIF_Open(PDEVICE_OBJECT pDeviceObject)
{
   NTSTATUS ntStatus;
   PDEVICE_EXTENSION pDevExt;

   pDevExt = pDeviceObject->DeviceExtension;

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> -->USBIF_Open Rml[0]=%u\n", pDevExt->PortName, pDevExt->Sts.lRmlCount[0])
   );

   if (pDevExt->bInService == TRUE)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _Open: STATUS_DEVICE_BUSY\n", pDevExt->PortName)
      );
      return STATUS_DEVICE_BUSY;
   }

   if (inDevState(DEVICE_STATE_DEVICE_REMOVED0 | DEVICE_STATE_SURPRISE_REMOVED))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _Open: STATUS_DELETE_PENDING 0\n", pDevExt->PortName)
      );
      return STATUS_DELETE_PENDING;
   }

   if (!inDevState(DEVICE_STATE_PRESENT_AND_STARTED))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _Open: STATUS_DELETE_PENDING 1\n", pDevExt->PortName)
      );
      return STATUS_DELETE_PENDING;
   }

   if ((pDevExt->MuxInterface.MuxEnabled != 0x01) || 
       (pDevExt->MuxInterface.InterfaceNumber == pDevExt->MuxInterface.PhysicalInterfaceNumber))
   {
   ntStatus = QCUSB_ResetInput(pDeviceObject, QCUSB_RESET_PIPE_AND_ENDPOINT);
   if (ntStatus == STATUS_SUCCESS)
   {
      ntStatus = QCUSB_ResetOutput(pDeviceObject, QCUSB_RESET_PIPE_AND_ENDPOINT);
   }
   else // if (ntStatus != STATUS_SUCCESS)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _Open: ResetInput err 0x%x\n", pDevExt->PortName, ntStatus)
      );
      return STATUS_UNSUCCESSFUL;
   }
   }

   setDevState(DEVICE_STATE_CLIENT_PRESENT);

   if ((pDevExt->MuxInterface.MuxEnabled != 0x01) || 
       (pDevExt->MuxInterface.InterfaceNumber == pDevExt->MuxInterface.PhysicalInterfaceNumber))
   {
   // Set interface active when bus has no traffic
   QCUSB_CDC_SetInterfaceIdle
   (
      pDevExt->MyDeviceObject,
      pDevExt->DataInterface,
      FALSE,
      0
   );
   }

   pDevExt->bInService = TRUE;
   ntStatus = USBRD_Enqueue(pDevExt, NULL, 0);  // start read threads
   if (!NT_SUCCESS(ntStatus))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _Open: StartRead err 0x%x\n", pDevExt->PortName, ntStatus)
      );
      return ntStatus;
   }

   ntStatus = USBWT_Enqueue(pDevExt, NULL, 0);  // start write threads
   if (!NT_SUCCESS(ntStatus))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _Open: StartWrite err 0x%x\n", pDevExt->PortName, ntStatus)
      );
      return ntStatus;
   }

   // Toggle the DTR
   USBCTL_ClrDtrRts(pDeviceObject);
   USBCTL_SetDtr(pDeviceObject);

   if (pDevExt->InServiceSelectiveSuspension == FALSE)
   {
      QCPWR_CancelIdleTimer(pDevExt, 0, TRUE, 2);
   }

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> <--USBIF_Open Rml[0]=%u\n", pDevExt->PortName, pDevExt->Sts.lRmlCount[0])
   );
   return STATUS_SUCCESS;
} // USBIF_Open

NTSTATUS USBIF_Close(PDEVICE_OBJECT pDO)
{
   PDEVICE_EXTENSION pDevExt;
   NTSTATUS ntStatus = STATUS_SUCCESS;

   pDevExt = pDO->DeviceExtension;

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> -->USBIF_Close Rml[0]=%u\n", pDevExt->PortName, pDevExt->Sts.lRmlCount[0])
   );

   if (pDevExt->Sts.lRmlCount[0] == 0)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _Close: ERROR 0\n", pDevExt->PortName)
      );
   }

   if (pDevExt->bInService == FALSE)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _Close: Already out of service\n", pDevExt->PortName)
      );
   }

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_INFO,
      ("<%s> _Close: Clear DTR/RTS at port close\n", pDevExt->PortName)
   );
   USBCTL_ClrDtrRts(pDO);

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_INFO,
      ("<%s> _Close: USBRD_CancelReadThread\n", pDevExt->PortName)
   );
   ntStatus = USBRD_CancelReadThread(pDevExt, 0);
   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_INFO,
      ("<%s> _Close: USBWT_CancelWriteThread\n", pDevExt->PortName)
   );
   ntStatus = USBWT_CancelWriteThread(pDevExt, 2);

   clearDevState( DEVICE_STATE_CLIENT_PRESENT );

   pDevExt->bInService = FALSE; // show device closed

   ntStatus = CancelNotificationIrp(pDO);
   USBUTL_CleanupReadWriteQueues(pDevExt);

   // Set interface idle when bus has no traffic
   QCUSB_CDC_SetInterfaceIdle
   (
      pDevExt->MyDeviceObject,
      pDevExt->DataInterface,
      TRUE,
      0
   );

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> <--USBIF_Close Rml[0]=%u\n", pDevExt->PortName, pDevExt->Sts.lRmlCount[0])
   );
   return STATUS_SUCCESS;
}  // USBIF_Close


NTSTATUS USBIF_CancelWriteThread(PDEVICE_OBJECT pDO)
{
   PDEVICE_EXTENSION pDevExt;
   NTSTATUS ntStatus = STATUS_SUCCESS;

   pDevExt = pDO->DeviceExtension;


   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_INFO,
      ("<%s> USBIF_CancelWriteThread: USBWT_CancelWriteThread\n", pDevExt->PortName)
   );
   ntStatus = USBWT_CancelWriteThread(pDevExt, 2);

   USBUTL_PurgeQueue
   (
      pDevExt,
      &pDevExt->WriteDataQueue,
      &pDevExt->WriteSpinLock,
      QCUSB_IRP_TYPE_WIRP,
      1
   );

   return STATUS_SUCCESS;
}  // USBIF_Close

NTSTATUS USBIF_NdisVendorRegistryProcess
(
   IN PVOID          MiniportContext,
   IN NDIS_HANDLE    WrapperConfigurationContext
)
{
   NTSTATUS       ntStatus  = STATUS_SUCCESS;
   HANDLE         hRegKey = NULL;
   UNICODE_STRING ucValueName;
   ULONG          ulReturnLength;
   ULONG          gDriverConfigParam = 0;

   NDIS_STATUS                     ndisStatus = NDIS_STATUS_FAILURE;
   NDIS_HANDLE                     configurationHandle = NULL;
   PNDIS_CONFIGURATION_PARAMETER   pReturnedValue = NULL;
   NDIS_CONFIGURATION_PARAMETER    ReturnedValue;
   PMP_ADAPTER    pAdapter = (PMP_ADAPTER)MiniportContext;

   #ifdef NDIS60_MINIPORT

   NDIS_CONFIGURATION_OBJECT ConfigObject;

   ConfigObject.Header.Type = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
   ConfigObject.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
   ConfigObject.Header.Size = NDIS_SIZEOF_CONFIGURATION_OBJECT_REVISION_1;
   ConfigObject.NdisHandle = pAdapter->AdapterHandle;
   ConfigObject.Flags = 0;

   #endif // NDIS60_MINIPORT

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
   gVendorConfig.UseMultiReads       = TRUE;   // enabled by default
   gVendorConfig.UseMultiWrites      = TRUE;   // enabled by default

   #ifdef NDIS60_MINIPORT
   if (QCMP_NDIS6_Ok == TRUE)
   {
      ndisStatus = NdisOpenConfigurationEx
                   (
                      &ConfigObject,
                      &configurationHandle
                   );
   }
   #else
   NdisOpenConfiguration
   (
      &ndisStatus,
      &configurationHandle,
      WrapperConfigurationContext
   );
   #endif // NDIS60_MINIPORT

   if (ndisStatus != NDIS_STATUS_SUCCESS)
   {
      // emit error
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> NdisOpenConfig: ERR 0x%x\n", gDeviceName, ndisStatus)
      );
      return STATUS_UNSUCCESSFUL;
   }

   {
      UNICODE_STRING DriverVersionUnicode;
      ntStatus = USBUTL_NdisConfigurationGetString
                 (
                    configurationHandle,
                    NULL,
                    (UCHAR)NDIS_DEV_VER,
                    &DriverVersionUnicode,
                    1
                 );

      if (ntStatus != STATUS_SUCCESS)
      {
         QCUSB_DbgPrintG
         (
            QCUSB_DBG_MASK_CONTROL,
            QCUSB_DBG_LEVEL_ERROR,
            ("<%s> CfgGetStr: reg ERR\n", gDeviceName)
         );
      }
      else
      {
         ANSI_STRING ansiStr;
         RtlUnicodeStringToAnsiString(&ansiStr, &DriverVersionUnicode, TRUE);
         strcpy(gVendorConfig.DriverVersion, ansiStr.Buffer);
         RtlFreeAnsiString(&ansiStr);
         _freeUcBuf(DriverVersionUnicode);
      }
   }
   // Get number of retries on error
   ntStatus = USBUTL_NdisConfigurationGetValue
              (
                 configurationHandle,
                 (UCHAR)NDIS_RTY_NUM,
                 &gVendorConfig.NumOfRetriesOnError,
                 1
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
   ntStatus = USBUTL_NdisConfigurationGetValue
              (
                 configurationHandle,
                 (UCHAR)NDIS_MAX_XFR,
                 &gVendorConfig.MaxPipeXferSize,
                 1
              );

   if (ntStatus != STATUS_SUCCESS)
   {
      gVendorConfig.MaxPipeXferSize = QCUSB_RECEIVE_BUFFER_SIZE;
   }

   // Get number of L2 buffers
   ntStatus = USBUTL_NdisConfigurationGetValue
              (
                 configurationHandle,
                 (UCHAR)NDIS_L2_BUFS,
                 &gVendorConfig.NumberOfL2Buffers,
                 1
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
   ntStatus = USBUTL_NdisConfigurationGetValue
              (
                 configurationHandle,
                 (UCHAR)NDIS_DBG_MASK,
                 &gVendorConfig.DebugMask,
                 1
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
   ntStatus = USBUTL_NdisConfigurationGetValue
              (
                 configurationHandle,
                 (UCHAR)NDIS_DEV_CONFIG,
                 &gDriverConfigParam,
                 1
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
         gVendorConfig.Use2048ByteInPkt = TRUE;
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
      if (gDriverConfigParam & QCUSB_USE_MULTI_READS)
      {
         gVendorConfig.UseMultiReads = TRUE;
      }
      if (gDriverConfigParam & QCUSB_USE_MULTI_WRITES)
      {
         gVendorConfig.UseMultiWrites = TRUE;
      }
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


   // Get Thread Priority
   ntStatus = USBUTL_NdisConfigurationGetValue
              (
                 configurationHandle,
                 (UCHAR)NDIS_THREAD_PRI,
                 &gVendorConfig.ThreadPriority,
                 1
              );

   if (ntStatus != STATUS_SUCCESS)
   {
      gVendorConfig.ThreadPriority = 26;
   }
   else
   {
       if ( gVendorConfig.ThreadPriority < 10)
       {
          gVendorConfig.ThreadPriority = 10;
       }
       if ( gVendorConfig.ThreadPriority > HIGH_PRIORITY)
       {
          gVendorConfig.ThreadPriority = HIGH_PRIORITY;
       }
   }
   
   NdisCloseConfiguration(configurationHandle);

   return STATUS_SUCCESS;
} // USBIF_NdisVendorRegistryProcess

NTSTATUS USBIF_NdisPostVendorRegistryProcess
(
   IN PVOID          MiniportContext,
   IN NDIS_HANDLE    WrapperConfigurationContext,
   IN PDEVICE_OBJECT DeviceObject
)
{
   NTSTATUS                      ntStatus  = STATUS_SUCCESS;
   PDEVICE_EXTENSION             pDevExt = DeviceObject->DeviceExtension;
   NDIS_STATUS                   ndisStatus = NDIS_STATUS_FAILURE;
   NDIS_HANDLE                   configurationHandle = NULL;
   PNDIS_CONFIGURATION_PARAMETER pReturnedValue = NULL;
   NDIS_CONFIGURATION_PARAMETER  ReturnedValue;
   PMP_ADAPTER                   pAdapter = (PMP_ADAPTER)MiniportContext;

   #ifdef NDIS60_MINIPORT

   NDIS_CONFIGURATION_OBJECT ConfigObject;

   ConfigObject.Header.Type = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
   ConfigObject.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
   ConfigObject.Header.Size = NDIS_SIZEOF_CONFIGURATION_OBJECT_REVISION_1;
   ConfigObject.NdisHandle = pAdapter->AdapterHandle;
   ConfigObject.Flags = 0;

   #endif // NDIS60_MINIPORT

   pDevExt->bLoggingOk = FALSE;

   _zeroUnicode(pDevExt->ucLoggingDir);

   #ifdef NDIS60_MINIPORT
   if (QCMP_NDIS6_Ok == TRUE)
   {
      ndisStatus = NdisOpenConfigurationEx
                   (
                      &ConfigObject,
                      &configurationHandle
                   );
   }
   #else   
   NdisOpenConfiguration
   (
      &ndisStatus,
      &configurationHandle,
      WrapperConfigurationContext
   );
   #endif // NDIS60_MINIPORT
   if (ndisStatus != NDIS_STATUS_SUCCESS)
   {
      // emit error
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> NdisOpenConfig: ERR-1 0x%x\n", pDevExt->PortName, ndisStatus)
      );
      return STATUS_UNSUCCESSFUL;
   }

   ntStatus = USBUTL_NdisConfigurationGetString
              (
                 configurationHandle,
                 pDevExt,
                 (UCHAR)NDIS_LOG_DIR,
                 &pDevExt->ucLoggingDir,
                 1
              );

   if (ntStatus == STATUS_SUCCESS)
   {
      pDevExt->bLoggingOk = TRUE;
      dbgPrintUnicodeString(&pDevExt->ucLoggingDir,"pDevExt->ucLoggingDir");
   }

   NdisCloseConfiguration(configurationHandle);

   return STATUS_SUCCESS;
}  // USBIF_NdisPostVendorRegistryProcess

VOID USBIF_SetDebugMask(PDEVICE_OBJECT DO, char *buffer)
{
   PDEVICE_EXTENSION pDevExt;
   ULONG UsbDebugMask;

   if (DO == NULL)
   {
      return;
   }

   UsbDebugMask = *((ULONG*)buffer);
   pDevExt = DO->DeviceExtension;

   RtlCopyMemory(buffer, (void*)&(pDevExt->DebugMask), sizeof(ULONG));

   pDevExt->DebugMask = UsbDebugMask;
   pDevExt->DebugLevel = (UCHAR)(UsbDebugMask & 0x0F);

   gVendorConfig.DebugMask = pDevExt->DebugMask;
   gVendorConfig.DebugLevel = pDevExt->DebugLevel;

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_FORCE,
      ("<%s> USB debug mask set to 0x%x(L%d)\n", pDevExt->PortName,
        pDevExt->DebugMask, pDevExt->DebugLevel)
   );
}  // USBIF_SetDebugMask

NTSTATUS USBIF_SetPowerState
(
   PDEVICE_OBJECT DO,
   NDIS_DEVICE_POWER_STATE PowerState
)
{
   PDEVICE_EXTENSION pDevExt;

   if (DO == NULL)
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_FORCE,
         ("<%s> _SetPowerState: NIL DO\n", gDeviceName)
      );
      return STATUS_UNSUCCESSFUL;
   }

   pDevExt = DO->DeviceExtension;

   switch (PowerState)
   {
      case NdisDeviceStateUnspecified:
         pDevExt->PowerState = PowerDeviceUnspecified;
         pDevExt->bEncStopped = FALSE;
         KeSetEvent
         (
            &pDevExt->DispatchWakeupEvent,
            IO_NO_INCREMENT,
            FALSE
         );
         break;
      case NdisDeviceStateD0:
         pDevExt->bEncStopped = FALSE;
         if ((pDevExt->PowerState == NdisDeviceStateD1) ||
             (pDevExt->PowerState == NdisDeviceStateD2) ||
             (pDevExt->PowerState == NdisDeviceStateD3))
         {
            if ((pDevExt->PowerSuspended == TRUE) &&
                (pDevExt->SelectiveSuspended == FALSE))
            {
               KeSetEvent
               (
                  &pDevExt->DispatchWakeupResetEvent,
                  IO_NO_INCREMENT,
                  FALSE
               );
            }
            else
            {
               KeSetEvent
               (
                  &pDevExt->DispatchWakeupEvent,
                  IO_NO_INCREMENT,
                  FALSE
               );
            }
         }
         else
         {
            KeSetEvent
            (
               &pDevExt->DispatchWakeupEvent,
               IO_NO_INCREMENT,
               FALSE
            );
         }
         pDevExt->PowerState = PowerDeviceD0;
         break;
      case NdisDeviceStateD1:
         pDevExt->PowerState = PowerDeviceD1;
         pDevExt->bEncStopped = TRUE;
         KeSetEvent
         (
            &pDevExt->DispatchStandbyEvent,
            IO_NO_INCREMENT,
            FALSE
         );
         break;
      case NdisDeviceStateD2:
         pDevExt->PowerState = PowerDeviceD2;
         pDevExt->bEncStopped = TRUE;
         KeSetEvent
         (
            &pDevExt->DispatchStandbyEvent,
            IO_NO_INCREMENT,
            FALSE
         );
         break;
      case NdisDeviceStateD3:
         pDevExt->PowerState = PowerDeviceD3;
         pDevExt->bEncStopped = TRUE;
         KeSetEvent
         (
            &pDevExt->DispatchStandbyEvent,
            IO_NO_INCREMENT,
            FALSE
         );
         break;
      default:
         break;
   }

   // need to cleanup (consolidate the two members)
   pDevExt->DevicePower = pDevExt->PowerState;

   MPMAIN_DevicePowerStateChange(pDevExt->MiniportContext, PowerState);

   return STATUS_SUCCESS;

}  // USBIF_SetPowerState

UCHAR USBIF_GetDataInterfaceNumber(PDEVICE_OBJECT DO)
{
   PDEVICE_EXTENSION pDevExt;

   if (DO == NULL)
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_FORCE,
         ("<%s> _GetDataInterfaceNumber: NIL DO\n", gDeviceName)
      );
      return 0xFF;
   }

   pDevExt = DO->DeviceExtension;

   return pDevExt->DataInterface;
}  // USBIF_GetDataInterfaceNumber

SYSTEM_POWER_STATE USBIF_GetSystemPowerState(PDEVICE_OBJECT DO)
{
   PDEVICE_EXTENSION pDevExt;

   if (DO == NULL)
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_FORCE,
         ("<%s> _GetDataInterfaceNumber: NIL DO\n", gDeviceName)
      );
      return PowerSystemUnspecified;
   }
   pDevExt = DO->DeviceExtension;

   return USBPWR_GetSystemPowerState(pDevExt, QCUSB_SYS_POWER_CURRENT);

}  // USBIF_GetSystemPowerState

VOID USBIF_PollDevice(PDEVICE_OBJECT DO, BOOLEAN Active)
{
   PDEVICE_EXTENSION pDevExt;

   if (DO == NULL)
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_FORCE,
         ("<%s> _PollDevice: NIL DO\n", gDeviceName)
      );
      return;
   }
   pDevExt = DO->DeviceExtension;

   if (Active == FALSE)
   {
      KeSetEvent
      (
         &pDevExt->DispatchStopPollingEvent,
         IO_NO_INCREMENT,
         FALSE
      );
      // pDevExt->InServiceSelectiveSuspension = TRUE;  // can suspend
   }
   else
   {
      KeSetEvent
      (
         &pDevExt->DispatchStartPollingEvent,
         IO_NO_INCREMENT,
         FALSE
      );
      // pDevExt->InServiceSelectiveSuspension = FALSE;  // no suspend
   }
} // USBIF_PollDevice

VOID USBIF_PowerDownDevice
(
   PVOID              DeviceExtension,
   DEVICE_POWER_STATE DevicePower
)
{
   PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)DeviceExtension;

   MPMAIN_PowerDownDevice(pDevExt->MiniportContext, DevicePower);
}  // USBIF_PowerDownDevice

VOID USBIF_SyncPowerDownDevice
(
   PVOID              DeviceExtension,
   DEVICE_POWER_STATE DevicePower
)
{
   PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)DeviceExtension;
   DEVICE_POWER_STATE pwrState = PowerDeviceD3;
   NDIS_DEVICE_POWER_STATE ndisPwrState = NdisDeviceStateD3;

   if (DevicePower < PowerDeviceD3)
   {
      pwrState = PowerDeviceD2;
      ndisPwrState = NdisDeviceStateD2;
   }
   pDevExt->PowerState = pwrState;
   pDevExt->bEncStopped = TRUE;

   USBMAIN_StopDataThreads(pDevExt, FALSE);

   // need to cleanup (consolidate the two members)
   pDevExt->DevicePower = pDevExt->PowerState;

   MPMAIN_DevicePowerStateChange(pDevExt->MiniportContext, ndisPwrState);
}  // USBIF_SyncPowerDownDevice

VOID USBIF_SetupDispatchFilter(PVOID DriverObject)
{
   PDRIVER_OBJECT drv = (PDRIVER_OBJECT)DriverObject;
   int i;

   for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
   {
      QCDriverDispatchTable[i] = drv->MajorFunction[i];
      if ((i == IRP_MJ_POWER) && (drv->MajorFunction[i] != NULL))
      {
         drv->MajorFunction[i] = USBIF_DispatchFilter;
      }
   }
}  // USBIF_SetupDispatchFilter

NTSTATUS USBIF_DispatchFilter
(
   IN PDEVICE_OBJECT DO,
   IN PIRP Irp
)
{
   PIO_STACK_LOCATION irpStack;
   BOOLEAN            bFiltered = FALSE;
   PMP_ADAPTER        pAdapter;
   PDEVICE_EXTENSION  pDevExt;
   NDIS_DEVICE_POWER_STATE pwrState = NdisDeviceStateUnspecified;

   if (DO == NULL)
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> DispatchFilter: NIL DO\n", gDeviceName)
      );
      Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
      return STATUS_NOT_IMPLEMENTED;
   }

   irpStack = IoGetCurrentIrpStackLocation(Irp);

   #ifdef QCUSB_DBGPRINT
   QCUSB_DbgPrintG
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_INFO,
      ("<%s> DispatchFilter: to relay IRP[0x%X, 0x%X] 0x%p DO 0x%p\n", gDeviceName,
          irpStack->MajorFunction, irpStack->MinorFunction, Irp, DO)
   );
   #endif

   if ((irpStack->MinorFunction == IRP_MN_SET_POWER) &&
       (irpStack->Parameters.Power.Type == SystemPowerState))
   {
      GlobalData.LastSysPwrIrpState = irpStack->Parameters.Power.State.SystemState;
   }

   pAdapter = USBIF_FindNdisContext(DO);
   if (pAdapter == NULL)
   {
      #ifdef QCUSB_DBGPRINT
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> DispatchFilter: ERR- to NDIS IRP 0x%p Current SP %d\n",
                gDeviceName, Irp, GlobalData.LastSysPwrIrpState)
      );
      #endif
      goto send_to_ndis;
   }
   pDevExt = pAdapter->USBDo->DeviceExtension;
   bFiltered = USBIF_IsPowerIrpOriginatedFromUSB(pAdapter->USBDo, Irp);

   if (irpStack->MajorFunction == IRP_MJ_POWER)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_POWER,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> DispatchFilter: PWR IRP 0x%p\n", pDevExt->PortName, Irp)
      );
      switch (irpStack->MinorFunction)
      {
         case IRP_MN_WAIT_WAKE:
         {
            QCUSB_DbgPrint
            (
               QCUSB_DBG_MASK_POWER,
               QCUSB_DBG_LEVEL_DETAIL,
               ("<%s> DispatchFilter: IRP_MN_WAIT_WAKE 0x%p/0x%p\n", pDevExt->PortName,
                 Irp, pDevExt->WaitWakeIrp)
            );
            bFiltered = TRUE;  // TODO: need to further verify aginst pDevExt->WaitWakeIrp
            break;
         }

         case IRP_MN_QUERY_POWER:
         {
            switch (irpStack->Parameters.Power.Type)
            { 
               case SystemPowerState:
               {
                  QCUSB_DbgPrintG
                  (
                     QCUSB_DBG_MASK_POWER,
                     QCUSB_DBG_LEVEL_DETAIL,
                     ("<%s> DispatchFilter: QRY-sysPwr S%d\n", gDeviceName,
                       (irpStack->Parameters.Power.State.SystemState-1))
                  );
                  if (irpStack->Parameters.Power.State.SystemState <= PowerSystemWorking)
                  {
                     pAdapter->IsMipderegSent = FALSE;
                     QCUSB_DbgPrintG
                     (
                        QCUSB_DBG_MASK_POWER,
                        QCUSB_DBG_LEVEL_DETAIL,
                        ("<%s> DispatchFilter: Resetting IsMipderegSent %d\n", gDeviceName,
                          (pAdapter->IsMipderegSent))
                     );
                     pAdapter->ToPowerDown = pDevExt->PrepareToPowerDown = FALSE;
                  }
                  if (irpStack->Parameters.Power.State.SystemState >= PowerSystemSleeping3)
                  {
                     CleanupTxQueues(pAdapter);
                     
                     if (pAdapter->IsMipderegSent != TRUE)
                     {
                        if (pAdapter->Deregister == 1)
                        {
                           // Send Dereg request if not already sent
                           MPQMUX_SendDeRegistration(pAdapter);
                        }
                        pAdapter->IsMipderegSent = TRUE;
                        QCUSB_DbgPrintG
                        (
                           QCUSB_DBG_MASK_POWER,
                           QCUSB_DBG_LEVEL_DETAIL,
                           ("<%s> DispatchFilter: setting IsMipderegSent %d\n", gDeviceName,
                             (pAdapter->IsMipderegSent))
                        );
                     }
                     pAdapter->ToPowerDown = pDevExt->PrepareToPowerDown = TRUE;
                  }
                  break;
               }
               case DevicePowerState:
               {
                  QCUSB_DbgPrintG
                  (
                     QCUSB_DBG_MASK_POWER,
                     QCUSB_DBG_LEVEL_DETAIL,
                     ("<%s> DispatchFilter: QRY-devPwr D%d\n", gDeviceName,
                       (irpStack->Parameters.Power.State.DeviceState-1))
                  );
                  break;
               }
               default:
               {
                  DbgPrint("<%s> DispatchFilter: QRY-unknownPwr\n", gDeviceName);
                  break;
               }
            }
            break;
         }
         case IRP_MN_SET_POWER:
         {
            switch (irpStack->Parameters.Power.Type)
            { 
               case SystemPowerState:
               {
                  QCUSB_DbgPrintG
                  (
                     QCUSB_DBG_MASK_POWER,
                     QCUSB_DBG_LEVEL_DETAIL,
                     ("<%s> DispatchFilter: SET-sysPwr S%d\n", gDeviceName,
                       (irpStack->Parameters.Power.State.SystemState-1))
                  );

                  if (irpStack->Parameters.Power.State.SystemState <= PowerSystemWorking)
                  {
                     pAdapter->IsMipderegSent = FALSE;
                     QCUSB_DbgPrintG
                     (
                        QCUSB_DBG_MASK_POWER,
                        QCUSB_DBG_LEVEL_DETAIL,
                        ("<%s> DispatchFilter: Resetting IsMipderegSent %d\n", gDeviceName,
                          (pAdapter->IsMipderegSent))
                     );
                     pAdapter->ToPowerDown = pDevExt->PrepareToPowerDown = FALSE;
                  }
                  if (irpStack->Parameters.Power.State.SystemState >= PowerSystemSleeping3)
                  {
                     CleanupTxQueues(pAdapter);
                     
                     if (pAdapter->IsMipderegSent != TRUE)
                     {
                         if (pAdapter->Deregister == 1)
                         {
                            // Send Dereg request if not already sent
                            MPQMUX_SendDeRegistration(pAdapter);
                         }
                        pAdapter->IsMipderegSent = TRUE;
                        QCUSB_DbgPrintG
                        (
                           QCUSB_DBG_MASK_POWER,
                           QCUSB_DBG_LEVEL_DETAIL,
                           ("<%s> DispatchFilter: setting IsMipderegSent %d\n", gDeviceName,
                             (pAdapter->IsMipderegSent))
                        );
                     }
                     pAdapter->ToPowerDown = pDevExt->PrepareToPowerDown = TRUE;
                  }
 
                  pAdapter->MiniportSystemPower = pDevExt->SystemPower =
                     irpStack->Parameters.Power.State.SystemState;
                  
                  break;
               }
               case DevicePowerState:
               {
                  QCUSB_DbgPrintG
                  (
                     QCUSB_DBG_MASK_POWER,
                     QCUSB_DBG_LEVEL_DETAIL,
                     ("<%s> DispatchFilter: SET-devPwr D%d\n", gDeviceName,
                       irpStack->Parameters.Power.State.DeviceState-1)
                  );
                  pAdapter->MiniportDevicePower =
                     irpStack->Parameters.Power.State.DeviceState;
                  switch (irpStack->Parameters.Power.State.DeviceState)
                  {
                     case PowerDeviceD0:
                        pwrState = NdisDeviceStateD0;
                        break;
                     case PowerDeviceD1:
                        pwrState = NdisDeviceStateD1;
                        break;
                     case PowerDeviceD2:
                        pwrState = NdisDeviceStateD2;
                        break;
                     case PowerDeviceD3:
                        pwrState = NdisDeviceStateD3;
                        break;
                     default:
                        pwrState = NdisDeviceStateUnspecified;
                        break;
                  }
                  break;
               }
               default:
               {
                  QCUSB_DbgPrintG
                  (
                     QCUSB_DBG_MASK_POWER,
                     QCUSB_DBG_LEVEL_DETAIL,
                     ("<%s> DispatchFilter: SET-unknownPwr\n", gDeviceName)
                  );
                  break;
               }
            }
            break;
         }
         default:
         {
            QCUSB_DbgPrintG
            (
               QCUSB_DBG_MASK_POWER,
               QCUSB_DBG_LEVEL_DETAIL,
               ("<%s> DispatchFilter: unknownPwr MN\n", gDeviceName)
            );
            break;
         }
      }  // switch MN
   }  // if MJ

   if (bFiltered == TRUE)
   {
      return QCDirectIoCallDriver(pAdapter->USBDo, Irp);
   }

send_to_ndis:

   if (QCDriverDispatchTable[irpStack->MajorFunction] != NULL)
   {
      return (*QCDriverDispatchTable[irpStack->MajorFunction])(DO, Irp);
   }
   else
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_POWER,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> DispatchFilter: empty disp entry\n", gDeviceName)
      );
      Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
      IoCompleteRequest(Irp, IO_NO_INCREMENT);
   }

   return STATUS_NOT_IMPLEMENTED;
}  // USBIF_DispatchFilter

VOID USBIF_AddFdoMap
(
   PDEVICE_OBJECT NdisFdo,
   PVOID          MiniportContext
)
{
   PQC_NDIS_USB_FDO_MAP fdoMap;

   fdoMap = ExAllocatePoolWithTag
            (
               NonPagedPool,
               sizeof(QC_NDIS_USB_FDO_MAP),
               'PAMQ'
            );
   if (fdoMap == NULL)
   {
      return;
   }

   fdoMap->NdisFDO = NdisFdo;
   fdoMap->MiniportContext = MiniportContext;

   NdisAcquireSpinLock(&GlobalData.Lock);
   InsertTailList(&MPUSB_FdoMap, &(fdoMap->List));
   NdisReleaseSpinLock(&GlobalData.Lock);
   QCUSB_DbgPrintG
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> USBIF_AddFdoMap\n", gDeviceName)
   );
}  // USBIF_AddFdoMap

VOID USBIF_CleanupFdoMap(PDEVICE_OBJECT NdisFdo)
{
   PLIST_ENTRY          headOfList, peekEntry;
   PQC_NDIS_USB_FDO_MAP fdoMap;

   NdisAcquireSpinLock(&GlobalData.Lock);

   if (NdisFdo == NULL)
   {
      while (!IsListEmpty(&MPUSB_FdoMap))
      {
         headOfList = RemoveHeadList(&MPUSB_FdoMap);
         fdoMap = CONTAINING_RECORD
                  (
                     headOfList,
                     QC_NDIS_USB_FDO_MAP,
                     List
                  );
         QCUSB_DbgPrintG
         (
            QCUSB_DBG_MASK_CONTROL,
            QCUSB_DBG_LEVEL_DETAIL,
            ("<%s> CleanupFdoMap: RM 0x%p\n", gDeviceName, fdoMap)
         );
         ExFreePool(fdoMap);
      }
   }
   else
   {
      if (!IsListEmpty(&MPUSB_FdoMap))
      {
         headOfList = &MPUSB_FdoMap;
         peekEntry = headOfList->Flink;
         while (peekEntry != headOfList)
         {
            fdoMap = CONTAINING_RECORD
                     (
                        peekEntry,
                        QC_NDIS_USB_FDO_MAP,
                        List
                     );
            if (fdoMap->NdisFDO == NdisFdo)
            {
               QCUSB_DbgPrintG
               (
                  QCUSB_DBG_MASK_CONTROL,
                  QCUSB_DBG_LEVEL_DETAIL,
                  ("<%s> CleanupFdoMap: rm 0x%p\n", gDeviceName, fdoMap)
               );
               RemoveEntryList(peekEntry);
               ExFreePool(fdoMap);
               break;
            }
            peekEntry = peekEntry->Flink;
         }
      }
   }

   NdisReleaseSpinLock(&GlobalData.Lock);
}  // USBIF_CleanupFdoMap

PVOID USBIF_FindNdisContext(PDEVICE_OBJECT NdisFdo)
{
   PQC_NDIS_USB_FDO_MAP fdoMap;
   PVOID                foundContext = NULL;
   PLIST_ENTRY          headOfList, peekEntry;

   NdisAcquireSpinLock(&GlobalData.Lock);
   if (!IsListEmpty(&MPUSB_FdoMap))
   {
      headOfList = &MPUSB_FdoMap;
      peekEntry = headOfList->Flink;

      while (peekEntry != headOfList)
      {
         fdoMap = CONTAINING_RECORD
                  (
                     peekEntry,
                     QC_NDIS_USB_FDO_MAP,
                     List
                  );
         if (fdoMap->NdisFDO == NdisFdo)
         {
            foundContext = fdoMap->MiniportContext;
            break;
         }
         peekEntry = peekEntry->Flink;
      }
   }
   NdisReleaseSpinLock(&GlobalData.Lock);

   QCUSB_DbgPrintG
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> _FindNdisContext:  0x%p\n", gDeviceName, foundContext)
   );
   return foundContext;

}  // USBIF_FindNdisContext

BOOLEAN USBIF_IsPowerIrpOriginatedFromUSB
(
   PDEVICE_OBJECT UsbFDO,
   PIRP           Irp
)
{
   PDEVICE_EXTENSION  pDevExt;
   PIO_STACK_LOCATION irpStack;
   PLIST_ENTRY        headOfList, peekEntry;
   BOOLEAN            result = FALSE;
   #ifdef QCSER_TARGET_XP
   KLOCK_QUEUE_HANDLE levelOrHandle;
   #else
   KIRQL levelOrHandle;
   #endif

   pDevExt = (PDEVICE_EXTENSION)UsbFDO->DeviceExtension;
   irpStack = IoGetCurrentIrpStackLocation(Irp);

   // Self-requested IRP must be SET-POWER IRP
   if (irpStack->MinorFunction != IRP_MN_SET_POWER)
   {
      return FALSE;
   }
   if (irpStack->Parameters.Power.Type != DevicePowerState)
   {
      return FALSE;
   }

   QcAcquireSpinLock(&pDevExt->SingleIrpSpinLock, &levelOrHandle);
   if (!IsListEmpty(&pDevExt->OriginatedPwrReqQueue))
   {
      PSELF_ORIGINATED_POWER_REQ pwrReq;

      headOfList = &pDevExt->OriginatedPwrReqQueue;
      peekEntry = headOfList->Flink;

      while (peekEntry != headOfList)
      {
         pwrReq = CONTAINING_RECORD
                  (
                     peekEntry,
                     SELF_ORIGINATED_POWER_REQ,
                     List
                  );
         if (pwrReq->PwrState.DeviceState ==
             irpStack->Parameters.Power.State.DeviceState)
         {
            RemoveEntryList(peekEntry);
            InsertTailList(&pDevExt->OriginatedPwrReqPool, &(pwrReq->List));
            result = TRUE;
            break;
         }
         peekEntry = peekEntry->Flink;
      }
   }
   QcReleaseSpinLock(&pDevExt->SingleIrpSpinLock, levelOrHandle);

   QCUSB_DbgPrintG
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> IsPowerIrpOriginatedFromUSB:  %d\n", gDeviceName, result)
   );
   return result;

}  // USBIF_IsPowerIrpOriginatedFromUSB

BOOLEAN USBIF_IsUsbBroken(PDEVICE_OBJECT UsbFDO)
{
   PDEVICE_EXTENSION  pDevExt;
   PMP_ADAPTER pAdapter;

   pDevExt = (PDEVICE_EXTENSION)UsbFDO->DeviceExtension;
   pAdapter = pDevExt->MiniportContext;

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> _IsUsbBroken: ID %d PWR %d (0x%x, 0x%x)\n", pDevExt->PortName,
        pAdapter->InstanceIdx, GlobalData.LastSysPwrIrpState,
        pDevExt->ControlPipeStatus, pDevExt->IntPipeStatus)
   );

   if ((GlobalData.LastSysPwrIrpState != PowerSystemWorking) &&
       (GlobalData.LastSysPwrIrpState != PowerSystemUnspecified))
   {
      return FALSE;
   }

   /*****
   if (pAdapter->InstanceIdx == 2)
   {
      if (FALSE == USBRD_InPipeOk(pDevExt))
      {
         return TRUE;
      }
   }
   *****/

   if ((pDevExt->ControlPipeStatus == STATUS_DEVICE_NOT_CONNECTED) ||
       (pDevExt->IntPipeStatus     == STATUS_DEVICE_NOT_CONNECTED) ||
       (pDevExt->InputPipeStatus   == STATUS_DEVICE_NOT_CONNECTED) ||
       (pDevExt->OutputPipeStatus  == STATUS_DEVICE_NOT_CONNECTED) ||
       (pDevExt->XwdmStatus        == STATUS_DEVICE_NOT_CONNECTED))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> _IsUsbBroken: TRUE 1\n", pDevExt->PortName)
      );
      return TRUE;
   }

   if ((pDevExt->ControlPipeStatus == STATUS_NO_SUCH_DEVICE) ||
       (pDevExt->IntPipeStatus     == STATUS_NO_SUCH_DEVICE) ||
       (pDevExt->InputPipeStatus   == STATUS_NO_SUCH_DEVICE) ||
       (pDevExt->OutputPipeStatus  == STATUS_NO_SUCH_DEVICE) ||
       (pDevExt->XwdmStatus        == STATUS_NO_SUCH_DEVICE))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> _IsUsbBroken: TRUE 2\n", pDevExt->PortName)
      );
      return TRUE;
   }

   return FALSE;
} // USBIF_IsUsbBroken

VOID USBIF_SetAggregationMode(PDEVICE_OBJECT UsbFDO)
{
   PDEVICE_EXTENSION  pDevExt;
   PMP_ADAPTER pAdapter;

   pDevExt = (PDEVICE_EXTENSION)UsbFDO->DeviceExtension;
   pAdapter = pDevExt->MiniportContext;

#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif

#if defined(QCMP_QMAP_V1_SUPPORT)   
   pDevExt->QMAPEnabledV1 = pAdapter->QMAPEnabledV1;
   QCUSB_DbgPrint
   ( 
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> QMAP:%d\n", pDevExt->PortName, pDevExt->QMAPEnabledV1)
   );
#endif
   
#if defined(QCMP_MBIM_DL_SUPPORT)   
   pDevExt->MBIMDLEnabled = pAdapter->MBIMDLEnabled;
   QCUSB_DbgPrint
   ( 
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> MBIMDL:%d\n", pDevExt->PortName, pDevExt->MBIMDLEnabled)
   );
#endif
#if defined(QCMP_MBIM_UL_SUPPORT)   
   pDevExt->MBIMULEnabled = pAdapter->MBIMULEnabled;
   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> MBIMUL:%d\n", pDevExt->PortName, pDevExt->MBIMULEnabled)
   );
#endif
#if defined(QCMP_DL_TLP)   
   if (pAdapter->TLPDLEnabled == TRUE)
   {
      pDevExt->bEnableDataBundling[QC_LINK_DL] = TRUE;
   }
#endif
#if defined(QCMP_QMAP_V1_SUPPORT)   
   pDevExt->QMAPDLMinPadding = pAdapter->QMAPDLMinPadding;
   QCUSB_DbgPrint
   ( 
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> QMAPDLMinPadding:%d\n", pDevExt->PortName, pDevExt->QMAPDLMinPadding)
   );
#endif
}  // USBIF_SetDataMode


unsigned short CheckSum(unsigned short *buffer, int count)
{
   /* Compute Internet Checksum for "count" bytes
   *         beginning at location "buffer".
   */

   unsigned long cksum=0;

   while(count >1)
   {
      /*  This is the inner loop */
      cksum += *buffer++;
      count -= sizeof(unsigned short);
   }

   /*  Add left-over byte, if any */
   if(count > 0)
   {
      cksum += *(unsigned char*)buffer;

   }
   /*  Fold 32-bit sum to 16 bits */
   cksum = (cksum >> 16) + (cksum & 0xffff);
   cksum += (cksum >>16);

   /*  One final inversion to get the true checksum value */
   // cksum = ~cksum;

   return (unsigned short) cksum;
}

#ifdef QC_IP_MODE

VOID USBIF_SetDataMode(PDEVICE_OBJECT UsbFDO)
{
   PDEVICE_EXTENSION  pDevExt;
   PMP_ADAPTER pAdapter;

   pDevExt = (PDEVICE_EXTENSION)UsbFDO->DeviceExtension;
   pAdapter = pDevExt->MiniportContext;

   pDevExt->IPOnlyMode = pAdapter->IPModeEnabled;

   #ifdef NDIS620_MINIPORT

   if (QCMP_NDIS620_Ok == TRUE)
   {
      if (pAdapter->NdisMediumType == NdisMediumWirelessWan)
      {
         // no ETH manipulation with NDIS620
         pDevExt->IPOnlyMode = FALSE;
         pDevExt->WWANMode = TRUE;
      }
   }

   #endif // NDIS620_MINIPORT

   pDevExt->QoSMode    = pAdapter->QosEnabled;
   RtlCopyMemory
   (
      pDevExt->EthHdr.DstMacAddress, 
      pAdapter->CurrentAddress,
      ETH_LENGTH_OF_ADDRESS
   );
   RtlCopyMemory
   (
      pDevExt->EthHdr.SrcMacAddress, 
      pAdapter->MacAddress2,
      ETH_LENGTH_OF_ADDRESS
   );
   if (pAdapter->IPSettings.IPV4.Address != 0)
   {
      pDevExt->EthHdr.EtherType = ntohs(ETH_TYPE_IPV4);
   }
   else if (pAdapter->IPSettings.IPV6.Flags != 0)
   {
      pDevExt->EthHdr.EtherType = ntohs(ETH_TYPE_IPV6);
   }
   else
   {
      // error, temporarily use IP type
      pDevExt->EthHdr.EtherType = ntohs(ETH_TYPE_IPV4);
   }

   USBUTL_PrintBytes
   (
      (PVOID)&(pDevExt->EthHdr),
      sizeof(QC_ETH_HDR),
      sizeof(QC_ETH_HDR),
      "IPO: RSP_ETH_HDR",
      pDevExt,
      (QCUSB_DBG_MASK_CONTROL | QCUSB_DBG_MASK_RDATA),
      QCUSB_DBG_LEVEL_DETAIL
   );

   QCUSB_DbgPrintG
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> IPO: _SetDataMode: IPMode %d QOS %d\n", gDeviceName,
        pDevExt->IPOnlyMode, pDevExt->QoSMode)
   );
}  // USBIF_SetDataMode

VOID USBIF_NotifyLinkDown(PDEVICE_OBJECT UsbFDO)
{
   PDEVICE_EXTENSION pDevExt;
   PMP_ADAPTER pAdapter;
   
   pDevExt = (PDEVICE_EXTENSION)UsbFDO->DeviceExtension;
   pAdapter = pDevExt->MiniportContext;
   pAdapter->IsQMIOutOfService = TRUE;

   MPMAIN_DisconnectNotification(pAdapter);
} // USBIF_NotifyLinkDown

#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif // QCUSB_MUX_PROTOCOL

#endif // QC_IP_MODE

NTSTATUS USBIF_DataIrpCompletionRoutine
(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP           Irp,
   IN PVOID          Context
)
{
   // free memory allocations
   ExFreePool(Irp->AssociatedIrp.SystemBuffer);
   IoFreeIrp(Irp);
   return STATUS_MORE_PROCESSING_REQUIRED;
}  // USBIF_DataIrpCompletionRoutine

#if defined(QCMP_QMAP_V2_SUPPORT) || defined(QCMP_QMAP_V1_SUPPORT)

VOID USBIF_QMAPAddtoControlQueue(PDEVICE_OBJECT UsbFDO, PQCQMAPCONTROLQUEUE pQMAPControl)
{
   PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)UsbFDO->DeviceExtension;
   PMP_ADAPTER pAdapter = pDevExt->MiniportContext;
   NdisAcquireSpinLock( &pAdapter->CtrlWriteLock );
   InsertTailList( &pAdapter->QMAPControlList, &(pQMAPControl->QMAPListEntry));
   NdisReleaseSpinLock( &pAdapter->CtrlWriteLock );                               
   KeSetEvent(&pAdapter->MPThreadQMAPControlEvent, IO_NO_INCREMENT, FALSE);
}

VOID USBIF_QMAPProcessControlQueue(PDEVICE_OBJECT UsbFDO)
{
   PLIST_ENTRY pList;
   PQCQMAPCONTROLQUEUE QmapControlElement;    
   PQCQMAP_FLOWCONTROL_STRUCT pFlowCotrol;
   int listSize;
   PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)UsbFDO->DeviceExtension;
   PMP_ADAPTER pAdapter = pDevExt->MiniportContext;
   
   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> -->MP_ResolveRequests\n", pAdapter->PortName)
   );
   
   // Get control of the Control lists
   NdisAcquireSpinLock( &pAdapter->CtrlWriteLock );

   // While there are items in the Completed list
   while( !IsListEmpty( &pAdapter->QMAPControlList) )
   {
      // Remove the itme form the list and point to the pOID record
      pList = RemoveHeadList( &pAdapter->QMAPControlList );

      // For now we can release the CtrlRead list
      NdisReleaseSpinLock( &pAdapter->CtrlWriteLock );
      QmapControlElement = CONTAINING_RECORD( pList, QCQMAPCONTROLQUEUE, QMAPListEntry);
      pFlowCotrol = (PQCQMAP_FLOWCONTROL_STRUCT)QmapControlElement->Buffer;
      
      QCUSB_DbgPrint
      (
      QCUSB_DBG_MASK_READ,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> RIC : QMAP: Received Control message Command Type %d\n", pDevExt->PortName, pFlowCotrol->CommandName)
      );
      
      if ( (pFlowCotrol->CommandName == QMAP_FLOWCONTROL_COMMAND_PAUSE) && 
           (pFlowCotrol->CmdType == 0x00))
      {
         MPMAIN_QMAPFlowControlCall(pAdapter, TRUE);
         USBIF_QMAPSendFlowControlCommand(pDevExt->MyDeviceObject, pFlowCotrol->TransactionId, pFlowCotrol->CommandName, pFlowCotrol);
      }
      else if ( (pFlowCotrol->CommandName == QMAP_FLOWCONTROL_COMMAND_RESUME) && 
           (pFlowCotrol->CmdType == 0x00))
      {
         MPMAIN_QMAPFlowControlCall(pAdapter, FALSE);
         USBIF_QMAPSendFlowControlCommand(pDevExt->MyDeviceObject, pFlowCotrol->TransactionId, pFlowCotrol->CommandName, pFlowCotrol);
      }
      else if (pFlowCotrol->CmdType != 0x00)
      {
          QCUSB_DbgPrint
          (
          QCUSB_DBG_MASK_READ,
          QCUSB_DBG_LEVEL_DETAIL,
          ("<%s> RIC : QMAP: Received ACK response for the command sent\n", pDevExt->PortName)
          );
      }
      else
      {
         QCUSB_DbgPrint
         (
         QCUSB_DBG_MASK_READ,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> RIC : QMAP: Bad Control message\n", pDevExt->PortName)
         );
      }
      NdisFreeMemory(QmapControlElement, sizeof(QCQMAPCONTROLQUEUE), 0 );
      NdisAcquireSpinLock( &pAdapter->CtrlWriteLock );   
   }  // while

   // Be sure to let go
   NdisReleaseSpinLock( &pAdapter->CtrlWriteLock );
}

VOID USBIF_QMAPProcessControlMessage(PVOID pDevExtVoid, PUCHAR BufferPtr, ULONG Size)
{
   PQCQMAP_FLOWCONTROL_STRUCT pFlowControl;
   PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevExtVoid;
   PMP_ADAPTER pAdapter = pDevExt->MiniportContext;
   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> -->USBIF_QMAPProcessControlMessage\n", pAdapter->PortName)
   );
   
   pFlowControl = (PQCQMAP_FLOWCONTROL_STRUCT)BufferPtr;
      
   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_READ,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> RIC : QMAP: Received Control message Command Type %d\n", pDevExt->PortName, pFlowControl->CommandName)
   );
      
   if ( Size != sizeof(QCQMAP_FLOWCONTROL_STRUCT))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_READ,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> RIC : QMAP: Bad Control message\n", pDevExt->PortName)
      );
      return;
   }
   
   if ( (pFlowControl->CommandName == QMAP_FLOWCONTROL_COMMAND_PAUSE) && 
        (pFlowControl->CmdType == 0x00))
   {
      MPMAIN_QMAPFlowControlCall(pAdapter, TRUE);
      USBIF_QMAPSendFlowControlCommand(pDevExt->MyDeviceObject, pFlowControl->TransactionId, pFlowControl->CommandName, pFlowControl);
   }
   else if ( (pFlowControl->CommandName == QMAP_FLOWCONTROL_COMMAND_RESUME) && 
             (pFlowControl->CmdType == 0x00))
   {
      USBIF_QMAPSendFlowControlCommand(pDevExt->MyDeviceObject, pFlowControl->TransactionId, pFlowControl->CommandName, pFlowControl);
      MPMAIN_QMAPFlowControlCall(pAdapter, FALSE);
   }
   else if (pFlowControl->CmdType != 0x00)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_READ,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> RIC : QMAP: Received ACK response for the command sent\n", pDevExt->PortName)
       );
   }
   else
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_READ,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> RIC : QMAP: Bad Control message\n", pDevExt->PortName)
      );
   }
}


BOOLEAN USBIF_QMAPSendFlowControlCommand(PDEVICE_OBJECT UsbFDO, ULONG TransactionId, UCHAR QMAPCommand, PVOID pInputFlowControl)
{
   PDEVICE_EXTENSION pDevExt;
   PMP_ADAPTER       pAdapter;

   PIO_STACK_LOCATION nextstack;
   PIRP               pIrp;
   PCHAR              pBuffer, dataPtr = NULL;
   LONG               dataLen = 0;
   NTSTATUS           ntStatus;
   PMP_ADAPTER        returnAdapter;
   PQCQMAP_STRUCT     pQMAPHeader;
   PQCQMAP_FLOWCONTROL_STRUCT pQMAPFlowControl;
   PQCQMAP_FLOWCONTROL_STRUCT pInputFlowCtrl = (PQCQMAP_FLOWCONTROL_STRUCT)pInputFlowControl;
   pDevExt = (PDEVICE_EXTENSION)UsbFDO->DeviceExtension;
   pAdapter = pDevExt->MiniportContext;

   QCNET_DbgPrint
   (
      QCUSB_DBG_MASK_WRITE,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> --> USBIF_QMAPSendFlowControlAck 0x%x\n", pDevExt->PortName, pDevExt->BulkPipeOutput)
   );

#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif
   {
      // allocate buffer
      dataLen = sizeof(QCQMAP_STRUCT) + sizeof(QCQMAP_FLOWCONTROL_STRUCT);
   }
   pBuffer = ExAllocatePoolWithTag(NonPagedPool, (dataLen+8), (ULONG)'PAMQ');
   if (pBuffer == NULL)
   {
       QCNET_DbgPrint
       (
          QCUSB_DBG_MASK_WRITE,
          QCUSB_DBG_LEVEL_ERROR,
          ("<%s> USBIF_QMAPSendFlowControlAck failed to alloc buf\n", pDevExt->PortName)
       );
       return FALSE;
   }

   // allocate IRP
   pIrp = IoAllocateIrp((CCHAR)(pDevExt->MyDeviceObject->StackSize), FALSE);
   if( pIrp == NULL )
   {
       QCNET_DbgPrint
       (
          QCUSB_DBG_MASK_WRITE,
          QCUSB_DBG_LEVEL_ERROR,
          ("<%s> USBIF_QMAPSendFlowControlAck failed to alloc IRP\n", pDevExt->PortName)
       );
       ExFreePool(pBuffer);
       return FALSE;
   }

   RtlZeroMemory(pBuffer, (dataLen+8));
   pQMAPHeader = (PQCQMAP_STRUCT)pBuffer;
   pQMAPHeader->PadCD = 0x80;
   pQMAPHeader->MuxId = pAdapter->MuxId;
   pQMAPHeader->PacketLen = sizeof(QCQMAP_FLOWCONTROL_STRUCT);
   pQMAPHeader->PacketLen = RtlUshortByteSwap(pQMAPHeader->PacketLen);
   
#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#endif
   {
      pQMAPFlowControl = (PQCQMAP_FLOWCONTROL_STRUCT)(pBuffer + sizeof(QCQMAP_STRUCT));
   }
   if (TransactionId != 0)
   {
      pQMAPFlowControl->CommandName = QMAPCommand;
      pQMAPFlowControl->TransactionId = TransactionId;
      pQMAPFlowControl->FlowCtrlSeqNumIp = pInputFlowCtrl->FlowCtrlSeqNumIp;
      pQMAPFlowControl->CmdType = 0x01;
      pQMAPFlowControl->QOSId = 0xFFFFFFFF;
   }
   else
   {
      pQMAPFlowControl->CommandName = QMAPCommand;
      pQMAPFlowControl->TransactionId = GetQMUXTransactionId(pAdapter);
      pQMAPFlowControl->TransactionId = RtlUlongByteSwap(pQMAPFlowControl->TransactionId);
      pQMAPFlowControl->FlowCtrlSeqNumIp = 0x00000000;
      pQMAPFlowControl->CmdType = 0x00;
      pQMAPFlowControl->QOSId = 0xFFFFFFFF;
   }

   pIrp->MdlAddress = NULL;
   pIrp->AssociatedIrp.SystemBuffer = pBuffer;
   nextstack = IoGetNextIrpStackLocation(pIrp);
   nextstack->MajorFunction = IRP_MJ_WRITE;
   nextstack->Parameters.Write.Length = dataLen;


   IoSetCompletionRoutine
   (
      pIrp,
      USBIF_DataIrpCompletionRoutine,
      NULL,
      TRUE,TRUE,TRUE
   );

   returnAdapter = FindStackDeviceObject(pAdapter);
   if (returnAdapter != NULL)
   {
      ntStatus = QCIoCallDriver(returnAdapter->USBDo, pIrp);
   }
   else
   {
      ntStatus = STATUS_UNSUCCESSFUL;
      pIrp->IoStatus.Status = ntStatus;
      USBIF_DataIrpCompletionRoutine(pAdapter->USBDo, pIrp, NULL);
   }

   QCNET_DbgPrint
   (
      QCUSB_DBG_MASK_WRITE,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> <-- USBIF_QMAPSendFlowControlAck 0x%x\n", pDevExt->PortName, ntStatus)
   );

   return (NT_SUCCESS(ntStatus));
}  // 

#endif

BOOLEAN USBIF_InjectPacket(PDEVICE_OBJECT UsbFDO, PUCHAR pPacket, ULONG Size)
{
   PDEVICE_EXTENSION pDevExt;
   PMP_ADAPTER       pAdapter;

   PIO_STACK_LOCATION nextstack;
   PIRP               pIrp;
   PCHAR              pBuffer, dataPtr = NULL;
   NTSTATUS           ntStatus;
   PMP_ADAPTER        returnAdapter;
   pDevExt = (PDEVICE_EXTENSION)UsbFDO->DeviceExtension;
   pAdapter = pDevExt->MiniportContext;

   QCNET_DbgPrint
   (
      QCUSB_DBG_MASK_WRITE,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> --> USBIF_InjectPacket 0x%x\n", pDevExt->PortName, pDevExt->BulkPipeOutput)
   );

   pBuffer = ExAllocatePoolWithTag(NonPagedPool, (Size), (ULONG)'PAMQ');
   if (pBuffer == NULL)
   {
       QCNET_DbgPrint
       (
          QCUSB_DBG_MASK_WRITE,
          QCUSB_DBG_LEVEL_ERROR,
          ("<%s> USBIF_InjectPacket failed to alloc buf\n", pDevExt->PortName)
       );
       return FALSE;
   }

   // allocate IRP
   pIrp = IoAllocateIrp((CCHAR)(pDevExt->MyDeviceObject->StackSize), FALSE);
   if( pIrp == NULL )
   {
       QCNET_DbgPrint
       (
          QCUSB_DBG_MASK_WRITE,
          QCUSB_DBG_LEVEL_ERROR,
          ("<%s> USBIF_InjectPacket failed to alloc IRP\n", pDevExt->PortName)
       );
       ExFreePool(pBuffer);
       return FALSE;
   }

   RtlZeroMemory(pBuffer, (Size));
   RtlCopyMemory(pBuffer, pPacket, Size);

   pIrp->MdlAddress = NULL;
   pIrp->AssociatedIrp.SystemBuffer = pBuffer;
   nextstack = IoGetNextIrpStackLocation(pIrp);
   nextstack->MajorFunction = IRP_MJ_WRITE;
   nextstack->Parameters.Write.Length = Size;


   IoSetCompletionRoutine
   (
      pIrp,
      USBIF_DataIrpCompletionRoutine,
      NULL,
      TRUE,TRUE,TRUE
   );

   returnAdapter = FindStackDeviceObject(pAdapter);
   if (returnAdapter != NULL)
   {
      ntStatus = QCIoCallDriver(returnAdapter->USBDo, pIrp);
   }
   else
   {
      ntStatus = STATUS_UNSUCCESSFUL;
      pIrp->IoStatus.Status = ntStatus;
      USBIF_DataIrpCompletionRoutine(pAdapter->USBDo, pIrp, NULL);
   }

   QCNET_DbgPrint
   (
      QCUSB_DBG_MASK_WRITE,
      QCUSB_DBG_LEVEL_TRACE,
      ("<%s> <-- USBIF_InjectPacket 0x%x\n", pDevExt->PortName, ntStatus)
   );

   return (NT_SUCCESS(ntStatus));
}  // 

VOID USBIF_SetDataMTU(PDEVICE_OBJECT UsbFDO, LONG MtuValue)
{
   PDEVICE_EXTENSION pDevExt;

   pDevExt = (PDEVICE_EXTENSION)UsbFDO->DeviceExtension;

   if (MtuValue == 0)
   {
      pDevExt->MTU  = ETH_MAX_PACKET_SIZE;  // set default
   }
   else
   {
      pDevExt->MTU  = MtuValue;
   }
}  // USBIF_SetDataMTU

PIRP GetAdapterReadDataItemIrp(PVOID pDevExt1, UCHAR MuxId, PVOID *pReturnDevExt, BOOLEAN *ReadQueueEmpty)
{
   PLIST_ENTRY     readQueueheadOfList, readQueuepeekEntry, readQueuenextEntry;
   PLIST_ENTRY     adapterheadOfList, adapterpeekEntry, adapternextEntry;
   PDEVICE_EXTENSION pTempDevExt;
   PMP_ADAPTER pTempAdapter;
#ifdef QCUSB_TARGET_XP
   KLOCK_QUEUE_HANDLE levelOrHandle;
#else
   KIRQL levelOrHandle;
#endif
   PIRP pIrp = NULL;
   PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevExt1;
   *pReturnDevExt = (PVOID)pDevExt;       
   *ReadQueueEmpty = FALSE;
   NdisAcquireSpinLock(&GlobalData.Lock);
   if (!IsListEmpty(&GlobalData.AdapterList))
   {
      adapterheadOfList = &GlobalData.AdapterList;
      adapterpeekEntry = adapterheadOfList->Flink;
      while (adapterpeekEntry != adapterheadOfList)
      {
         pTempAdapter = CONTAINING_RECORD
                       (
                          adapterpeekEntry,
                          MP_ADAPTER,
                          List
                       );
         pTempDevExt = (PDEVICE_EXTENSION)pTempAdapter->USBDo->DeviceExtension;

         if ((pTempAdapter->MuxId == MuxId) &&
             (pDevExt->MuxInterface.PhysicalInterfaceNumber == pTempDevExt->MuxInterface.PhysicalInterfaceNumber) &&
             (pTempDevExt->MuxInterface.FilterDeviceObj == pDevExt->MuxInterface.FilterDeviceObj))
         {
              NdisReleaseSpinLock(&GlobalData.Lock);

              if (pDevExt != pTempDevExt)
              {
                 // check completion status
                 QcAcquireSpinLock(&pTempDevExt->ReadSpinLock, &levelOrHandle);
              }
              
              // Peek
              if (!IsListEmpty(&pTempDevExt->ReadDataQueue))
              {
                 // headOfList = RemoveHeadList(&pDevExt->ReadDataQueue);
                 readQueueheadOfList = &pTempDevExt->ReadDataQueue;
                 readQueuepeekEntry = readQueueheadOfList->Flink;
                 pIrp =  CONTAINING_RECORD
                         (
                            readQueuepeekEntry,
                            IRP,
                            Tail.Overlay.ListEntry
                         );
              }           
              else
              {
                 *ReadQueueEmpty = TRUE;
              }
              *pReturnDevExt = pTempDevExt;
              if (pDevExt != pTempDevExt)
              {
                 QcReleaseSpinLock(&pTempDevExt->ReadSpinLock, levelOrHandle);
              }              
              return pIrp;
         }
         adapterpeekEntry = adapterpeekEntry->Flink;
      }
   }       
   NdisReleaseSpinLock(&GlobalData.Lock);
   return pIrp;
}


BOOLEAN IsEmptyAllReadQueue(PVOID pDevExt1)
{
   PLIST_ENTRY     readQueueheadOfList, readQueuepeekEntry, readQueuenextEntry;
   PLIST_ENTRY     adapterheadOfList, adapterpeekEntry, adapternextEntry;
   PDEVICE_EXTENSION pTempDevExt;
   PMP_ADAPTER pTempAdapter;
#ifdef QCUSB_TARGET_XP
   KLOCK_QUEUE_HANDLE levelOrHandle;
#else
   KIRQL levelOrHandle;
#endif
   PDEVICE_EXTENSION pDevExt = (PDEVICE_EXTENSION)pDevExt1;
   BOOLEAN returnval = TRUE;
   NdisAcquireSpinLock(&GlobalData.Lock);
   if (!IsListEmpty(&GlobalData.AdapterList))
   {
      adapterheadOfList = &GlobalData.AdapterList;
      adapterpeekEntry = adapterheadOfList->Flink;
      while (adapterpeekEntry != adapterheadOfList)
      {
         pTempAdapter = CONTAINING_RECORD
                       (
                          adapterpeekEntry,
                          MP_ADAPTER,
                          List
                       );
         pTempDevExt = (PDEVICE_EXTENSION)pTempAdapter->USBDo->DeviceExtension;

         if ((pTempDevExt->MuxInterface.MuxEnabled == 0x01) &&
             (pTempDevExt->MuxInterface.PhysicalInterfaceNumber == pDevExt->MuxInterface.InterfaceNumber) &&
             (pTempDevExt->MuxInterface.FilterDeviceObj == pDevExt->MuxInterface.FilterDeviceObj))
         {
              // check completion status
              QcAcquireSpinLock(&pTempDevExt->ReadSpinLock, &levelOrHandle);
              
              // Peek
              if (!IsListEmpty(&pTempDevExt->ReadDataQueue))
              {
                 QcReleaseSpinLock(&pTempDevExt->ReadSpinLock, levelOrHandle);   
                 NdisReleaseSpinLock(&GlobalData.Lock);
                 return FALSE;                 
              }
              
              QcReleaseSpinLock(&pTempDevExt->ReadSpinLock, levelOrHandle);
         }
         adapterpeekEntry = adapterpeekEntry->Flink;
      }
   }       
   NdisReleaseSpinLock(&GlobalData.Lock);
   return returnval;
}


