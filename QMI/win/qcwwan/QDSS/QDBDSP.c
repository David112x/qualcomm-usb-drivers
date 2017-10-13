/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                          Q D B D E V . C

GENERAL DESCRIPTION
  This is the file which contains dispatch functions for QDSS driver.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#include "QDBMAIN.h"
#include "QDBDSP.h"

VOID QDBDSP_IoDeviceControl
(
   WDFQUEUE   Queue,
   WDFREQUEST Request,
   size_t     OutputBufferLength,
   size_t     InputBufferLength,
   ULONG      IoControlCode
)
{ 
   NTSTATUS        ntStatus;
   PDEVICE_CONTEXT pDevContext;
   PFILE_CONTEXT   fileContext;
   PVOID           outBuffer;
   size_t          bufferSize = 0;

   UNREFERENCED_PARAMETER(OutputBufferLength);
   UNREFERENCED_PARAMETER(InputBufferLength);

   pDevContext = QdbDeviceGetContext(WdfIoQueueGetDevice(Queue));
   fileContext = QdbFileGetContext(WdfRequestGetFileObject(Request));

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> -->QDBDSP_IoDeviceControl: 0x%p code 0x%x\n", pDevContext->PortName,
        Request, IoControlCode)
   );

   if (fileContext->Type == QDB_FILE_TYPE_TRACE)
   {
      QDB_DbgPrint
      (
         QDB_DBG_MASK_CONTROL,
         QDB_DBG_LEVEL_DETAIL,
         ("<%s> QDBDSP_IoDeviceControl: [TRACE] 0x%p \n", pDevContext->PortName)
      );
   }
   else if (fileContext->Type == QDB_FILE_TYPE_DEBUG)
   {
      QDB_DbgPrint
      (
         QDB_DBG_MASK_CONTROL,
         QDB_DBG_LEVEL_DETAIL,
         ("<%s> QDBDSP_IoDeviceControl: [DEBUG] 0x%p \n", pDevContext->PortName)
      );
   }
   else if (fileContext->Type == QDB_FILE_TYPE_DPL)
   {
      QDB_DbgPrint
      (
         QDB_DBG_MASK_CONTROL,
         QDB_DBG_LEVEL_DETAIL,
         ("<%s> QDBDSP_IoDeviceControl: [DPL] 0x%p \n", pDevContext->PortName)
      );
   }

   switch (IoControlCode)
   {
      case IOCTL_QCDEV_DPL_STATS:
      {
         QDB_DbgPrint
         (
            QDB_DBG_MASK_CONTROL,
            QDB_DBG_LEVEL_DETAIL,
            ("<%s> QDBDSP_IoDeviceControl: IOCTL_QCDEV_DPL_STATS (IN/OUT %d/%d) \n",
              pDevContext->PortName, InputBufferLength, OutputBufferLength)
         );
         ntStatus = WdfRequestRetrieveOutputBuffer(Request, 0, &outBuffer, &bufferSize);
         if (NT_SUCCESS(ntStatus))
         {
            if (OutputBufferLength >= sizeof(QDB_STATS))
            {
               RtlCopyMemory(outBuffer, &(pDevContext->Stats), sizeof(QDB_STATS));
               WdfRequestSetInformation(Request, sizeof(QDB_STATS));
            }
            else
            {
               ntStatus = STATUS_BUFFER_TOO_SMALL;
            }
         }
         else
         {
            ntStatus = STATUS_UNSUCCESSFUL;
         }
 
         break;
      }

      default:
      {
         ntStatus = STATUS_INVALID_DEVICE_REQUEST;
         break;
      }
   }

   WdfRequestComplete(Request, ntStatus);

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> <--QDBDSP_IoDeviceControl: code 0x%x (NTS 0x%x)\n", pDevContext->PortName, IoControlCode, ntStatus)
   );
   return;
}  // QDBDSP_IoDeviceControl

VOID QDBDSP_IoStop
(
   WDFQUEUE   Queue,
   WDFREQUEST Request,
   ULONG      Flags
)
{
   PDEVICE_CONTEXT  pDevContext;

   pDevContext = QdbDeviceGetContext(WdfIoQueueGetDevice(Queue));

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> -->QDBDSP_IoStop: queue 0x%p req 0x%p action 0x%x\n", pDevContext->PortName, Queue, Request, Flags)
   );

   if (Flags & WdfRequestStopActionPurge)
   {
      QDB_DbgPrint
      (
         QDB_DBG_MASK_CONTROL,
         QDB_DBG_LEVEL_DETAIL,
         ("<%s> <--QDBDSP_IoStop: queue 0x%p req 0x%p - WdfRequestStopActionPurge\n", pDevContext->PortName, Queue, Request)
      );
      WdfRequestCancelSentRequest(Request);

      return;
   }

   if (Flags & WdfRequestStopActionSuspend)
   {
      QDB_DbgPrint
      (
         QDB_DBG_MASK_CONTROL,
         QDB_DBG_LEVEL_DETAIL,
         ("<%s> <--QDBDSP_IoStop: queue 0x%p req 0x%p - WdfRequestStopActionSuspend\n", pDevContext->PortName, Queue, Request)
      );
      WdfRequestStopAcknowledge(Request, FALSE);

      return;
   }

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> <--QDBDSP_IoStop: queue 0x%p req 0x%p action 0x%x\n", pDevContext->PortName, Queue, Request, Flags)
   );

   return;
}  // QDBDSP_IoStop

VOID QDBDSP_IoResume
(
   WDFQUEUE   Queue,
   WDFREQUEST Request
)
{
   PDEVICE_CONTEXT pDevContext;
   PFILE_CONTEXT   fileContext;

   pDevContext = QdbDeviceGetContext(WdfIoQueueGetDevice(Queue));
   fileContext = QdbFileGetContext(WdfRequestGetFileObject(Request));  // for debugging

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> -->QDBDSP_IoResume: File %d queue 0x%p req 0x%p\n", pDevContext->PortName,
         fileContext->Type, Queue, Request)
   );

   QDB_DbgPrint
   (
      QDB_DBG_MASK_CONTROL,
      QDB_DBG_LEVEL_TRACE,
      ("<%s> <--QDBDSP_IoResume: queue 0x%p req 0x%p\n", pDevContext->PortName, Queue, Request)
   );

   return;
}  // QDBDSP_IoResume
