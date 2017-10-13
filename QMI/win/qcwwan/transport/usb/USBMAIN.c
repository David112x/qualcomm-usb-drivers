/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             U S B M A I N . C

GENERAL DESCRIPTION
  This file contains entry functions for the USB driver.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/

// CopyRight (c) Smart Technology Enablers, 1999 - 2000, All Rights Reserved.

/*
 * Copyright (C) 1997 by SystemSoft Corporation, as an unpublished
 * work.  All rights reserved.  Contains confidential information
 * and trade secrets proprietary to SystemSoft Corporation.
 * Disassembly or decompilation prohibited.
 */

#include <stdio.h>
#include "USBMAIN.h"
#include "USBIOC.h"
#include "USBWT.h"
#include "USBRD.h"
#include "USBUTL.h"
#include "USBPNP.h"
#include "USBDSP.h"
#include "USBINT.h"

// EVENT_TRACING
#ifdef EVENT_TRACING

#include "MPWPP.h"               // Driver specific WPP Macros, Globals, etc 
#include "USBMAIN.tmh"

#endif   // EVENT_TRACING

// Global Data, allocated at init time

#ifdef DEBUG_MSGS
   BOOLEAN bDbgout = TRUE;  // switch to control Debug Messages
#endif // DEBUG_MSGS

BOOLEAN bPowerManagement = TRUE;  // global switch, OK

UNICODE_STRING gServicePath;
KEVENT   gSyncEntryEvent;
char     gDeviceName[255];
char     gServiceName[255];
int      gModemType = -1;
USHORT   ucThCnt = 0;
QCUSB_VENDOR_CONFIG gVendorConfig;
BOOLEAN  bAddNewDevice;
KSPIN_LOCK gGenSpinLock;

/****************************************************************************
 *
 * function: DriverEntry
 *
 * purpose:  Installable driver initialization entry point, called directly
 *           by the I/O system.
 *
 * arguments:DriverObject = pointer to the driver object
 *           RegistryPath = pointer to a unicode string representing the path
 *
 * returns:  NT status
 *
 */

#ifndef NDIS_WDM

NTSTATUS DriverEntry
(
   IN PDRIVER_OBJECT DriverObject,
   IN PUNICODE_STRING RegistryPath
)
{
   ULONG i;
   ANSI_STRING asDevName;
   char *p;
   NTSTATUS ntStatus;

   // Create dispatch points for device control, create, close.

   bAddNewDevice    = FALSE;
   gVendorConfig.DriverResident = 0;

   ntStatus = RtlUnicodeStringToAnsiString
              (
                 &asDevName,
                 RegistryPath,
                 TRUE
              );

   if ( !NT_SUCCESS( ntStatus ) )
   {
      KdPrint(("qcnet: Failure at DriverEntry.\n"));
      _failx;
      return STATUS_UNSUCCESSFUL;
   }

   KeInitializeSpinLock(&gGenSpinLock);
   KeInitializeEvent(&gSyncEntryEvent, SynchronizationEvent, TRUE);
   KeSetEvent(&gSyncEntryEvent,IO_NO_INCREMENT,FALSE);

   #ifdef QCUSB_SHARE_INTERRUPT
   USBSHR_Initialize();
   #endif // QCUSB_SHARE_INTERRUPT

   p = asDevName.Buffer + asDevName.Length - 1;
   while ( p > asDevName.Buffer )
   {
      if (*p == '\\')
      {
         p++;
         break;
      }
      p--;
   }

   RtlZeroMemory(gDeviceName, 255);
   RtlZeroMemory((PVOID)(&gVendorConfig), sizeof(QCUSB_VENDOR_CONFIG));
   if ((strlen(p) == 0) || (strlen(p) >= 255)) // invalid name
   {
      strcpy(gDeviceName, "qc_unknown");
   }
   strcpy(gDeviceName, p);
   RtlFreeAnsiString(&asDevName);

   gVendorConfig.DebugMask  = 0x30000074;
   gVendorConfig.DebugLevel = 4;
   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("\n<%s> DriverEntry (Build: %s/%s)\n", gDeviceName, __DATE__, __TIME__)
   );
   QCUSB_DbgPrint2
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("\n<%s> DriverEntry RegPath(%dB): %s\n", gDeviceName, asDevName.Length, asDevName.Buffer)
   );

   // Store the service path
   ntStatus = USBUTL_AllocateUnicodeString
              (
                 &gServicePath,
                 RegistryPath->Length,
                 "gServicePath"
              );
   if (ntStatus != STATUS_SUCCESS)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("\n<%s> DriverEntry: gServicePath failure", gDeviceName)
      );
      _zeroUnicode(gServicePath);
   }
   else
   {
      RtlCopyUnicodeString(&gServicePath, RegistryPath);
   }

   // Assign dispatch routines
   for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++)
   {
      DriverObject->MajorFunction[i] = USBDSP_DirectDispatch;
   }

   DriverObject->MajorFunction[IRP_MJ_WRITE]        = USBWT_Write;
   DriverObject->MajorFunction[IRP_MJ_READ]         = USBRD_Read;
   DriverObject->DriverUnload                       = USBMAIN_Unload;
   DriverObject->DriverExtension->AddDevice         = USBPNP_AddDevice;

// DriverObject->MajorFunction[IRP_MJ_CREATE]         = USBDSP_QueuedDispatch;
// DriverObject->MajorFunction[IRP_MJ_CLOSE]          = USBDSP_QueuedDispatch;
// DriverObject->MajorFunction[IRP_MJ_CLEANUP]        = USBDSP_QueuedDispatch;
   DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = USBDSP_QueuedDispatch;

   return STATUS_SUCCESS;
}  //DriverEntry

#endif // NDIS_WDM

NTSTATUS USBMAIN_IrpCompletionSetEvent
(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP Irp,
   IN PVOID Context
)
{
   ULONG ulResult;
   PKEVENT pEvent = Context;

   if(Irp->PendingReturned)
   {
      _IoMarkIrpPending(Irp);
   }

   ulResult = KeSetEvent(pEvent, IO_NO_INCREMENT, FALSE);

   // let our dispatch routine complete it
   return STATUS_MORE_PROCESSING_REQUIRED;
}

NTSTATUS _QcIoCompleteRequest(IN PIRP Irp, IN CCHAR  PriorityBoost)
{
   NTSTATUS nts = Irp->IoStatus.Status;

   if (nts == STATUS_PENDING)
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_CRITICAL,
         ("<%s> ERROR: IRP STATUS_PENDING\n", gDeviceName)
      );
      #ifdef DEBUG_MSGS
      KdBreakPoint();
      #endif
   }

   IoCompleteRequest(Irp, PriorityBoost);
   return nts;
}

NTSTATUS QCIoCompleteRequest(IN PIRP Irp, IN CCHAR  PriorityBoost)
{
   NTSTATUS nts = Irp->IoStatus.Status;
   PIO_STACK_LOCATION currentStack;
   PDEVICE_OBJECT devObj;
   PVOID ctxt;

   QCUSB_DbgPrintG
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_CRITICAL,
      ("<%s> QCIoCompleteRequest: IRP 0x%p\n", gDeviceName, Irp)
   );

   IoCompleteRequest(Irp, IO_NO_INCREMENT);

if (0)
{

   if (nts == STATUS_PENDING)
   {
      QCUSB_DbgPrintG
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_CRITICAL,
         ("<%s> ERROR: IRP STATUS_PENDING\n", gDeviceName)
      );
      #ifdef DEBUG_MSGS
      KdBreakPoint();
      #endif
   }
   currentStack = IoGetCurrentIrpStackLocation(Irp);
   devObj = currentStack->DeviceObject;
   ctxt = currentStack->Context;
   IoSkipCurrentIrpStackLocation(Irp);

   if (currentStack->CompletionRoutine != NULL)
   {
      currentStack->CompletionRoutine(devObj, Irp, ctxt); 
   }

   RtlZeroMemory(currentStack, sizeof(IO_STACK_LOCATION));
}

   return nts;
}  // QCIoCompleteRequest

NTSTATUS QcCompleteRequest(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info)
{
   Irp->IoStatus.Status = status;
   Irp->IoStatus.Information = info;
   QCIoCompleteRequest(Irp, IO_NO_INCREMENT);
   return status;
} // QcCompleteRequest

NTSTATUS QcCompleteRequest2(IN PIRP Irp, IN NTSTATUS status, IN ULONG_PTR info)
{
   Irp->IoStatus.Status = status;
   Irp->IoStatus.Information = info;
   IoCompleteRequest(Irp, IO_NO_INCREMENT);
   return status;
} // QcCompleteRequest

/*********************************************************************
 *
 * function:   QCUSB_CleanupDeviceExtensionBuffers
 *
 * purpose: 
 *
 * arguments:  DeviceObject = adr( device object )
 *
 * returns:    NT status
 *
 */

NTSTATUS QCUSB_CleanupDeviceExtensionBuffers(IN PDEVICE_OBJECT DeviceObject)
{
   PDEVICE_EXTENSION pDevExt;
   NTSTATUS ntStatus = STATUS_SUCCESS;
   USHORT wIndex, i;
   #ifdef QCUSB_MULTI_WRITES
   PLIST_ENTRY headOfList;
   PQCMWT_BUFFER pWtBuf;
   #endif // QCUSB_MULTI_WRITES
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
      ("<%s> CleanupDeviceExtension: enter\n", pDevExt->PortName)
   );

   USBUTL_CleanupReadWriteQueues(pDevExt);

   // Free memory allocated for descriptors
   if (pDevExt -> pUsbDevDesc)
   {
      ExFreePool( pDevExt -> pUsbDevDesc );
      pDevExt -> pUsbDevDesc = NULL;
   }
   if (pDevExt -> pUsbConfigDesc)
   {
      ExFreePool( pDevExt -> pUsbConfigDesc );
      pDevExt -> pUsbConfigDesc = NULL;
   }
   
   // Free up any interface structures in the device extension.
   for (wIndex = 0; wIndex < MAX_INTERFACE; wIndex++)
   {
      if (pDevExt->Interface[wIndex] != NULL)
      {
         ExFreePool(pDevExt->Interface[wIndex]);
         pDevExt->Interface[wIndex] = NULL;
      }
   }
   if (pDevExt->UsbConfigUrb != NULL)
   {
      ExFreePool(pDevExt->UsbConfigUrb);
      pDevExt->UsbConfigUrb = NULL;
   }

   _freeUcBuf(pDevExt->ucLoggingDir);
   _zeroUnicode(pDevExt->ucLoggingDir);
   if (pDevExt->TlpFrameBuffer.Buffer != NULL)
   {
      ExFreePool(pDevExt->TlpFrameBuffer.Buffer);
      pDevExt->TlpFrameBuffer.Buffer = NULL;
   }
   if (pDevExt->pucReadBufferStart)
   {
      ExFreePool(pDevExt->pucReadBufferStart);
      pDevExt->pucReadBufferStart = NULL;
   }
   if (pDevExt->pL2ReadBuffer != NULL)
   {
      for (i = 0; i < pDevExt->NumberOfL2Buffers; i++)
      {
         if (pDevExt->pL2ReadBuffer[i].Buffer != NULL)
         {
            ExFreePool(pDevExt->pL2ReadBuffer[i].Buffer);
            pDevExt->pL2ReadBuffer[i].Buffer = NULL;

            if (pDevExt->pL2ReadBuffer[i].Irp != NULL)
            {
               IoFreeIrp(pDevExt->pL2ReadBuffer[i].Irp);
               pDevExt->pL2ReadBuffer[i].Irp = NULL;
            }
         }
      }
      ExFreePool(pDevExt->pL2ReadBuffer);
      pDevExt->pL2ReadBuffer = NULL;
   }

   #ifdef QCUSB_MULTI_WRITES
   QcAcquireSpinLock(&pDevExt->WriteSpinLock, &levelOrHandle);
   while (!IsListEmpty(&pDevExt->MWriteIdleQueue))
   {
      headOfList = RemoveHeadList(&pDevExt->MWriteIdleQueue);
      pWtBuf = CONTAINING_RECORD(headOfList, QCMWT_BUFFER, List);
      if (pWtBuf->Irp != NULL)
      {
         IoFreeIrp(pWtBuf->Irp);
      }
      ExFreePool(pWtBuf);
   }
   QcReleaseSpinLock(&pDevExt->WriteSpinLock, levelOrHandle);
   #endif // QCUSB_MULTI_WRITES

   QcEmptyCompletionQueue
   (
      pDevExt,
      &pDevExt->RdCompletionQueue,
      &pDevExt->ReadSpinLock,
      QCUSB_IRP_TYPE_RIRP
   );
   QcEmptyCompletionQueue
   (
      pDevExt,
      &pDevExt->WtCompletionQueue,
      &pDevExt->WriteSpinLock,
      QCUSB_IRP_TYPE_WIRP
   );
   QcEmptyCompletionQueue
   (
      pDevExt,
      &pDevExt->CtlCompletionQueue,
      &pDevExt->ControlSpinLock,
      QCUSB_IRP_TYPE_CIRP
   );
   QcEmptyCompletionQueue
   (
      pDevExt,
      &pDevExt->SglCompletionQueue,
      &pDevExt->SingleIrpSpinLock,
      QCUSB_IRP_TYPE_CIRP
   );

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> CleanupDeviceExtension: Exit\n", pDevExt->PortName)
   );

   return ntStatus;
} // QCUSB_CleanupDeviceExtensionBuffers

NTSTATUS CancelNotificationIrp(PDEVICE_OBJECT pDevObj)
{
   PDEVICE_EXTENSION   pDevExt;
   NTSTATUS            ntStatus = STATUS_SUCCESS;
   PIRP                pIrp;
   #ifdef QCUSB_TARGET_XP
   KLOCK_QUEUE_HANDLE levelOrHandle;
   #else
   KIRQL levelOrHandle;
   #endif

   pDevExt = pDevObj->DeviceExtension;

   QcAcquireSpinLock(&pDevExt->SingleIrpSpinLock, &levelOrHandle);
   pIrp = pDevExt->pNotificationIrp;
   if ( pIrp )
   {

      if (IoSetCancelRoutine(pIrp, NULL) != NULL)
      {
         pDevExt->pNotificationIrp = NULL;
         pIrp->IoStatus.Status = STATUS_CANCELLED;
         pIrp->IoStatus.Information = 0;

         InsertTailList(&pDevExt->SglCompletionQueue, &pIrp->Tail.Overlay.ListEntry);
         KeSetEvent(&pDevExt->InterruptEmptySglQueueEvent, IO_NO_INCREMENT, FALSE);
      }
   }
   QcReleaseSpinLock(&pDevExt->SingleIrpSpinLock, levelOrHandle);

   return ntStatus;
}

/*********************************************************************
 *
 * function:   cancelAllIrps
 *
 * purpose:    clean up device object removal
 *
 * arguments:  pDevObj = adr(device object)
 *
 * returns:    none
 *
 */
VOID cancelAllIrps( PDEVICE_OBJECT pDevObj )
{
   PDEVICE_EXTENSION pDevExt;
   NTSTATUS ntStatus;

   pDevExt = pDevObj -> DeviceExtension;

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_INFO,
      ("<%s> cancelAllIrps - cancel R/W threads\n", pDevExt->PortName)
   );
   ntStatus = USBRD_CancelReadThread(pDevExt, 2);
   ntStatus = USBWT_CancelWriteThread(pDevExt, 1);

} //cancellAllIrps

/*********************************************************************
 *
 * function:   USBMAIN_Unload
 *
 * purpose:    Free all the allocated resources, and unload driver
 *
 * arguments:  DriverObject = adr(driver object)
 *
 * returns:    none
 *
 *
 *
 **********************************************************************/

VOID USBMAIN_Unload( IN PDRIVER_OBJECT DriverObject )
{
   NTSTATUS ntStatus = STATUS_SUCCESS;

   // Free any global resources
   _freeString(gServicePath);

   #ifdef QCUSB_SHARE_INTERRUPT
   USBSHR_FreeReadControlElement(NULL);
   #endif // QCUSB_SHARE_INTERRUPT

   DbgPrint("   ================================\n");
   DbgPrint("     Driver(%d) Unloaded by System\n", gModemType);
   DbgPrint("       Version: %-10s         \n", gVendorConfig.DriverVersion);
   DbgPrint("       Device:  %-10s         \n", gDeviceName);
   DbgPrint("       Port:    %-50s\n", gVendorConfig.PortName);
   DbgPrint("   ================================\n");
}  //Unload

VOID CancelNotificationRoutine( PDEVICE_OBJECT CalledDO, PIRP pIrp )
{
   KIRQL irql = KeGetCurrentIrql();
   PDEVICE_OBJECT pDevObj = CalledDO;
   PDEVICE_EXTENSION pDevExt = pDevObj->DeviceExtension;
   #ifdef QCUSB_TARGET_XP
   KLOCK_QUEUE_HANDLE levelOrHandle;
   #else
   KIRQL levelOrHandle;
   #endif

   IoReleaseCancelSpinLock(pIrp->CancelIrql);

   QcAcquireSpinLock(&pDevExt->SingleIrpSpinLock, &levelOrHandle);
   IoSetCancelRoutine(pIrp, NULL);  // not necessary
   ASSERT(pDevExt->pNotificationIrp==pIrp);
   pDevExt->pNotificationIrp = NULL;

   pIrp->IoStatus.Status = STATUS_CANCELLED;
   pIrp->IoStatus.Information = 0;

   InsertTailList(&pDevExt->SglCompletionQueue, &pIrp->Tail.Overlay.ListEntry);
   KeSetEvent(&pDevExt->InterruptEmptySglQueueEvent, IO_NO_INCREMENT, FALSE);

   QcReleaseSpinLock(&pDevExt->SingleIrpSpinLock, levelOrHandle);
} //  CancelNotificationRoutine

void QCUSB_DispatchDebugOutput
(
   PIRP               Irp,
   PIO_STACK_LOCATION irpStack,
   PDEVICE_OBJECT     CalledDO,
   PDEVICE_OBJECT     DeviceObject,
   KIRQL              irql
)
{
   PDEVICE_EXTENSION pDevExt = NULL;
   char msgBuf[256], *msg;

   msg = msgBuf;
   if (DeviceObject == NULL)
   {
      if (gVendorConfig.DebugLevel < QCUSB_DBG_LEVEL_DETAIL)
      {
         return;
      }
      sprintf(msg, "<%s 0x%p::0x%p:IRQL-%d:0x%p> ", gDeviceName, CalledDO, DeviceObject, irql, Irp);
   }
   else
   {
      pDevExt = DeviceObject->DeviceExtension;
      if (pDevExt->DebugLevel < QCUSB_DBG_LEVEL_DETAIL)
      {
         return;
      }
      sprintf(msg, "<%s 0x%p::0x%p:IRQL-%d:0x%p> ", pDevExt->PortName, CalledDO, DeviceObject, irql, Irp);
   }
   msg += strlen(msg);
   switch (irpStack->MajorFunction)
   {
      case IRP_MJ_CREATE:
         sprintf(msg, "IRP_MJ_CREATE");
         break;
      case IRP_MJ_CLOSE:
         sprintf(msg, "IRP_MJ_CLOSE");
         break;
      case IRP_MJ_CLEANUP:
         sprintf(msg, "IRP_MJ_CLEANUP");
         break;
      case IRP_MJ_DEVICE_CONTROL:
      {
         switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
         {
            case IOCTL_SERIAL_SET_BAUD_RATE:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_BAUD_RATE");
               break;
            case IOCTL_SERIAL_SET_QUEUE_SIZE:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_QUEUE_SIZE");
               break;
            case IOCTL_SERIAL_SET_LINE_CONTROL:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_LINE_CONTROL");
               break;
            case IOCTL_SERIAL_SET_BREAK_ON:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_BREAK_ON");
               break;
            case IOCTL_SERIAL_SET_BREAK_OFF:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_BREAK_OFF");
               break;
            case IOCTL_SERIAL_IMMEDIATE_CHAR:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_IMMEDIATE_CHAR");
               break;
            case IOCTL_SERIAL_SET_TIMEOUTS:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_TIMEOUTS");
               break;
            case IOCTL_SERIAL_GET_TIMEOUTS:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_GET_TIMEOUTS");
               break;
            case IOCTL_SERIAL_SET_DTR:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_DTR");
               break;
            case IOCTL_SERIAL_CLR_DTR:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_CLR_DTR");
               break;
            case IOCTL_SERIAL_RESET_DEVICE:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_RESET_DEVICE");
               break;
            case IOCTL_SERIAL_SET_RTS:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_RTS");
               break;
            case IOCTL_SERIAL_CLR_RTS:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_RTS");
               break;
            case IOCTL_SERIAL_SET_XOFF:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_XOFF");
               break;
            case IOCTL_SERIAL_SET_XON:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_XON");
               break;
            case IOCTL_SERIAL_GET_WAIT_MASK:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_GET_WAIT_MASK");
               break;
            case IOCTL_SERIAL_SET_WAIT_MASK:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_WAIT_MASK");
               break;
            case IOCTL_SERIAL_WAIT_ON_MASK:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_WAIT_ON_MASK");
               break;
            case IOCTL_SERIAL_PURGE:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_PURGE");
               break;
            case IOCTL_SERIAL_GET_BAUD_RATE:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_GET_BAUD_RATE");
               break;
            case IOCTL_SERIAL_GET_LINE_CONTROL:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_GET_LINE_CONTROL");
               break;
            case IOCTL_SERIAL_GET_CHARS:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_GET_CHARS");
               break;
            case IOCTL_SERIAL_SET_CHARS:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_CHARS");
               break;
            case IOCTL_SERIAL_GET_HANDFLOW:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_GET_HANDFLOW");
               break;
            case IOCTL_SERIAL_SET_HANDFLOW:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_HANDFLOW");
               break;
            case IOCTL_SERIAL_GET_MODEMSTATUS:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_GET_MODEMSTATUS");
               break;
            case IOCTL_SERIAL_GET_COMMSTATUS:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_GET_COMMSTATUS");
               break;
            case IOCTL_SERIAL_XOFF_COUNTER:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_XOFF_COUNTER");
               break;
            case IOCTL_SERIAL_GET_PROPERTIES:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_GET_PROPERTIES");
               break;
            case IOCTL_SERIAL_GET_DTRRTS:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_GET_DTRRTS");
               break;

            // begin_winioctl
            case IOCTL_SERIAL_LSRMST_INSERT:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_LSRMST_INSERT");
               break;
            case IOCTL_SERENUM_EXPOSE_HARDWARE:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERENUM_EXPOSE_HARDWARE");
               break;
            case IOCTL_SERENUM_REMOVE_HARDWARE:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERENUM_REMOVE_HARDWARE");
               break;
            case IOCTL_SERENUM_PORT_DESC:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERENUM_PORT_DESC");
               break;
            case IOCTL_SERENUM_GET_PORT_NAME:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERENUM_GET_PORT_NAME");
               break;
            // end_winioctl

            case IOCTL_SERIAL_CONFIG_SIZE:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_CONFIG_SIZE");
               break;
            case IOCTL_SERIAL_GET_COMMCONFIG:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_GET_COMMCONFIG");
               break;
            case IOCTL_SERIAL_SET_COMMCONFIG:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_COMMCONFIG");
               break;

            case IOCTL_SERIAL_GET_STATS:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_GET_STATS");
               break;
            case IOCTL_SERIAL_CLEAR_STATS:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_CLEAR_STATS");
               break;
            case IOCTL_SERIAL_GET_MODEM_CONTROL:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_GET_MODEM_CONTROL");
               break;
            case IOCTL_SERIAL_SET_MODEM_CONTROL:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_MODEM_CONTROL");
               break;
            case IOCTL_SERIAL_SET_FIFO_CONTROL:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_SERIAL_SET_FIFO_CONTROL");
               break;

            case IOCTL_QCDEV_WAIT_NOTIFY:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_QCDEV_WAIT_NOTIFY");
               break;
            case IOCTL_QCDEV_DRIVER_ID:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_QCDEV_DRIVER_ID");
               break;
            case IOCTL_QCDEV_GET_SERVICE_KEY:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_QCDEV_GET_SERVICE_KEY");
               break;
            case IOCTL_QCDEV_SEND_CONTROL:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_QCDEV_SEND_CONTROL");
               break;
            case IOCTL_QCDEV_READ_CONTROL:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_QCDEV_READ_CONTROL");
               break;
            case IOCTL_QCDEV_GET_HDR_LEN:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_QCDEV_GET_HDR_LEN");
               break;
            case IOCTL_QCDEV_LOOPBACK_DATA_PKT:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/IOCTL_QCDEV_LOOPBACK_DATA_PKT");
               break;
            default:
               sprintf(msg, "IRP_MJ_DEVICE_CONTROL/ControlCode_0x%x",
                       irpStack->Parameters.DeviceIoControl.IoControlCode);
         }
         break;
      }
      case IRP_MJ_INTERNAL_DEVICE_CONTROL:
      {
         switch (irpStack->Parameters.DeviceIoControl.IoControlCode)
         {
            case IOCTL_SERIAL_INTERNAL_DO_WAIT_WAKE:
               sprintf(msg, "IRP_MJ_INTERNAL_DEVICE_CONTROL/IOCTL_SERIAL_INTERNAL_DO_WAIT_WAKE");
               break;
            case IOCTL_SERIAL_INTERNAL_CANCEL_WAIT_WAKE:
               sprintf(msg, "IRP_MJ_INTERNAL_DEVICE_CONTROL/IOCTL_SERIAL_INTERNAL_CANCEL_WAIT_WAKE");
               break;
            case IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS:
               sprintf(msg, "IRP_MJ_INTERNAL_DEVICE_CONTROL/IOCTL_SERIAL_INTERNAL_BASIC_SETTINGS");
               break;
            case IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS:
               sprintf(msg, "IRP_MJ_INTERNAL_DEVICE_CONTROL/IOCTL_SERIAL_INTERNAL_RESTORE_SETTINGS");
               break;
            default:
               sprintf(msg, "IRP_MJ_INTERNAL_DEVICE_CONTROL/ControlCode_0x%x",
                       irpStack->Parameters.DeviceIoControl.IoControlCode);
         }
         break;
      }
      case IRP_MJ_POWER:
         switch (irpStack->MinorFunction)
         {
            case IRP_MN_WAIT_WAKE:
               sprintf(msg, "IRP_MJ_POWER/IRP_MN_WAIT_WAKE");
               break;
            case IRP_MN_POWER_SEQUENCE:
               sprintf(msg, "IRP_MJ_POWER/IRP_MN_POWER_SEQUENCE");
               break;
            case IRP_MN_SET_POWER:
               sprintf(msg, "IRP_MJ_POWER/IRP_MN_SET_POWER");
               break;
            case IRP_MN_QUERY_POWER:
               sprintf(msg, "IRP_MJ_POWER/IRP_MN_QUERY_POWER");
               break;
            default:
               sprintf(msg, "IRP_MJ_POWER/MinorFunc_0x%x", irpStack->MinorFunction);
         }
         break;
      case IRP_MJ_PNP:
      {
         switch (irpStack->MinorFunction)
         {
            case IRP_MN_QUERY_CAPABILITIES:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_QUERY_CAPABILITIES");
               break;
            case IRP_MN_START_DEVICE:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_START_DEVICE");
               break;
            case IRP_MN_QUERY_STOP_DEVICE:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_QUERY_STOP_DEVICE");
               break;
            case IRP_MN_STOP_DEVICE:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_STOP_DEVICE");
               break;
            case IRP_MN_QUERY_REMOVE_DEVICE:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_QUERY_REMOVE_DEVICE");
               break;
            case IRP_MN_SURPRISE_REMOVAL:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_SURPRISE_REMOVAL");
               break;
            case IRP_MN_REMOVE_DEVICE:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_REMOVE_DEVICE");
               break;
            case IRP_MN_QUERY_ID:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_QUERY_ID");
               break;
            case IRP_MN_QUERY_PNP_DEVICE_STATE:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_QUERY_PNP_DEVICE_STATE");
               break;
            case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_QUERY_RESOURCE_REQUIREMENTS");
               break;
            case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_FILTER_RESOURCE_REQUIREMENTS");
               break;
            case IRP_MN_QUERY_DEVICE_RELATIONS:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_QUERY_DEVICE_RELATIONS(0x%x)",
                       irpStack->Parameters.QueryDeviceRelations.Type);
               break;
            case IRP_MN_QUERY_LEGACY_BUS_INFORMATION:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_QUERY_LEGACY_BUS_INFORMATION");
               break;
            case IRP_MN_QUERY_INTERFACE:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_QUERY_INTERFACE");
               break;
            case IRP_MN_QUERY_DEVICE_TEXT:
               sprintf(msg, "IRP_MJ_PNP/IRP_MN_QUERY_DEVICE_TEXT");
               break;
            default:
               sprintf(msg, "IRP_MJ_PNP/MINOR_FUNCTION_0x%x",
                       irpStack->MinorFunction);
         }
         break;
      }
      case IRP_MJ_SYSTEM_CONTROL:
         sprintf(msg, "IRP_MJ_SYSTEM_CONTROL");
         break;
      default:
         sprintf(msg, "MAJOR_FUNCTIN: 0x%x", irpStack->MajorFunction);
         break;
   }
   sprintf(msg+strlen(msg), "\n");

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("%s",msgBuf)
   );
}  // QCUSB_DispatchDebugOutput

VOID USBMAIN_ResetRspAvailableCount
(
   PDEVICE_EXTENSION pDevExt,
   USHORT Interface,
   char *info,
   UCHAR cookie
)
{
   int i;

   // NOTE: pDevExt could be NULL

   QCUSB_DbgPrintX
   (
      pDevExt,
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> ResetRspAvailableCount - %d \n", info, cookie)
   );

   #ifdef QCUSB_SHARE_INTERRUPT
   USBSHR_ResetRspAvailableCount(pDevExt->MyDeviceObject, Interface);
   #else
   InterlockedExchange(&(pDevExt->lRspAvailableCount), 0);
   #endif // QCUSB_SHARE_INTERRUPT

}  // USBMAIN_ResetRspAvailableCount

NTSTATUS USBMAIN_StopDataThreads(PDEVICE_EXTENSION pDevExt, BOOLEAN CancelWaitWake)
{
   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> -->StopDataThreads\n", pDevExt->PortName)
   );

   if (pDevExt->bLowPowerMode == TRUE)
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_ERROR,
         ("<%s> _StopDataThreads: already in low power mode\n", pDevExt->PortName)
      );
      return STATUS_SUCCESS;
   }

   // D0 -> Dn, stop Device
   pDevExt->PowerSuspended = TRUE;
   pDevExt->bLowPowerMode = TRUE;

   QCUSB_CDC_SetInterfaceIdle
   (
      pDevExt->MyDeviceObject,
      pDevExt->DataInterface,
      TRUE,
      1
   );

   // If device powers down, drop DTR
   if ((pDevExt->PowerState >= PowerDeviceD3) &&
       ((pDevExt->SystemPower > PowerSystemWorking) ||
        (pDevExt->PrepareToPowerDown == TRUE)))
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> StopDataThreads: drop DTR\n", pDevExt->PortName)
      );
      USBCTL_ClrDtrRts(pDevExt->MyDeviceObject);
   }
   else
   {
      QCUSB_DbgPrint
      (
         QCUSB_DBG_MASK_CONTROL,
         QCUSB_DBG_LEVEL_DETAIL,
         ("<%s> StopDataThreads: still in S0, keep DTR\n", pDevExt->PortName)
      );
   }
   USBRD_SetStopState(pDevExt, TRUE, 7);   // blocking call
   USBWT_SetStopState(pDevExt, TRUE);      // non-blocking
   USBINT_StopInterruptService(pDevExt, TRUE, CancelWaitWake, 5);
   USBMAIN_ResetRspAvailableCount
   (
      pDevExt,
      pDevExt->DataInterface,
      pDevExt->PortName,
      6
   );

   QCUSB_DbgPrint
   (
      QCUSB_DBG_MASK_CONTROL,
      QCUSB_DBG_LEVEL_DETAIL,
      ("<%s> <--StopDataThreads\n", pDevExt->PortName)
   );

   return STATUS_SUCCESS;

}  // USBMAIN_StopDataThreads

