/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             Q C W T . H

GENERAL DESCRIPTION
  This file contains definitions for USB write operations.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#ifndef QCWT_H
#define QCWT_H

#include "QCMAIN.h"

#define QCWT_GPSC_START 0
#define QCWT_GPSC_STOP  1

#ifdef QCUSB_MUX_PROTOCOL
#error code not present
#else

#define QcExAllocateWriteIOB(_iob_,_uselock_) \
        { \
           KIRQL levelOrHandle; \
           if (_uselock_ == TRUE) \
           { \
              QcAcquireSpinLock(&pDevExt->WriteSpinLock, &levelOrHandle); \
           } \
           if (IsListEmpty(&pDevExt->WriteFreeQueue) == TRUE) \
           { \
              _iob_ = ExAllocatePool \
                      ( \
                         NonPagedPool, \
                         sizeof(VXD_WDM_IO_CONTROL_BLOCK) \
                      ); \
              if (_iob_ != NULL) {InterlockedIncrement(&(pDevExt->Sts.lAllocatedWtMem));} \
           } \
           else \
           { \
              PLIST_ENTRY headOfList; \
              headOfList = RemoveHeadList(&pDevExt->WriteFreeQueue); \
              _iob_ = CONTAINING_RECORD \
                      ( \
                         headOfList, \
                         VXD_WDM_IO_CONTROL_BLOCK, \
                         List \
                      ); \
           } \
           if (_iob_ != NULL) \
           { \
              InterlockedIncrement(&(pDevExt->Sts.lAllocatedWrites)); \
           } \
           if (_uselock_ == TRUE) \
           { \
              QcReleaseSpinLock(&pDevExt->WriteSpinLock, levelOrHandle); \
           } \
        }

#endif // QCUSB_MUX_PROTOCOL

#define QcExFreeWriteIOB(_iob_,_uselock_) \
        { \
           KIRQL levelOrHandle; \
           if (_uselock_ == TRUE) \
           { \
              QcAcquireSpinLock(&pDevExt->WriteSpinLock, &levelOrHandle); \
           } \
           InsertTailList(&pDevExt->WriteFreeQueue, &_iob_->List); \
           InterlockedDecrement(&(pDevExt->Sts.lAllocatedWrites)); \
           _iob_ = NULL; \
           if (_uselock_ == TRUE) \
           { \
              QcReleaseSpinLock(&pDevExt->WriteSpinLock, levelOrHandle); \
           } \
        }

NTSTATUS QCWT_Write(IN PDEVICE_OBJECT CalledDO, IN PIRP pIrp);
VOID     CancelWriteRoutine(PDEVICE_OBJECT DeviceObject, PIRP pIrp);
NTSTATUS WriteIrpCompletion
(
   PVXD_WDM_IO_CONTROL_BLOCK pIOBlock,
   BOOLEAN                   AllowSpinlock,
   UCHAR                     cookie
);
NTSTATUS CancelWriteThread(PDEVICE_EXTENSION pDevExt, UCHAR cookie);
NTSTATUS STESerial_Write(PVXD_WDM_IO_CONTROL_BLOCK pIOBlock);
NTSTATUS WriteCompletionRoutine
(
   PDEVICE_OBJECT pDO,
   PIRP           pIrp,
   PVOID          pContext
);
VOID     STESerial_WriteThread(PVOID pContext);
ULONG    CountWriteQueue (PDEVICE_EXTENSION pDevExt);
BOOLEAN  StartWriteTimeout(PVXD_WDM_IO_CONTROL_BLOCK pIOBlock);
VOID     AbortWriteTimeout(PVXD_WDM_IO_CONTROL_BLOCK pIOBlock);
VOID     WriteTimeoutDpc
(
   IN PKDPC Dpc,
   IN PVOID DeferredContext,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2
);
VOID TimeoutWriteRoutine
(
   PVXD_WDM_IO_CONTROL_BLOCK pIOBlock
);
PVXD_WDM_IO_CONTROL_BLOCK FindWriteIrp
(
   PDEVICE_EXTENSION pDevExt,
   PIRP              pIrp,
   BOOLEAN           IOBQueueOnly
);
VOID QCWT_ProcessPreTimeoutIOB(PDEVICE_EXTENSION pDevExt);

NTSTATUS QCWT_ImmediateChar(PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp);
NTSTATUS QCWT_PriorityWrite(PVXD_WDM_IO_CONTROL_BLOCK pIOBlock);
NTSTATUS QCWT_StartWriteThread(PDEVICE_EXTENSION pDevExt);
NTSTATUS QCWT_Suspend(PDEVICE_EXTENSION pDevExt);
NTSTATUS QCWT_Resume(PDEVICE_EXTENSION pDevExt);

#ifdef QCUSB_MULTI_WRITES
typedef struct _QCUSB_MWT_SENT_IRP
{
   PVXD_WDM_IO_CONTROL_BLOCK IOB;
   PIRP          SentIrp;
   ULONG         TotalLength; // for debugging
   PQCMWT_BUFFER MWTBuf;
   BOOLEAN       IrpReturned;
   LIST_ENTRY    List;
} QCUSB_MWT_SENT_IRP, *PQCUSB_MWT_SENT_IRP;

NTSTATUS QCMWT_WriteCompletionRoutine
(
   PDEVICE_OBJECT pDO,
   PIRP           pIrp,
   PVOID          pContext
);
VOID QCMWT_WriteThread(PVOID pContext);
NTSTATUS QCMWT_InitializeMultiWriteElements(PDEVICE_EXTENSION pDevExt);
PQCMWT_BUFFER QCMWT_IsIrpPending
(
   PDEVICE_EXTENSION pDevExt,
   PIRP              Irp,
   BOOLEAN           EnqueueToCancel
);
VOID QCMWT_CancelWriteRoutine(PDEVICE_OBJECT CalledDO, PIRP pIrp);
VOID QCMWT_TimeoutWriteRoutine
(
   PVXD_WDM_IO_CONTROL_BLOCK pIOBlock
);
BOOLEAN USBMWT_MarkAndCheckReturnedIrp
(
   PDEVICE_EXTENSION pDevExt,
   PQCMWT_BUFFER     MWTBuf,
   BOOLEAN           RemoveIrp,
   MWT_STATE         OperationState,
   NTSTATUS          Status
);
VOID QCMWT_TimeoutCompletion(PDEVICE_EXTENSION pDevExt);
PQCMWT_BUFFER QCMWT_IsIrpBeingCompleted
(
   PDEVICE_EXTENSION pDevExt,
   PIRP              Irp
);
#endif // QCUSB_MULTI_WRITES

NTSTATUS QCWT_GPSCIrpCompletionRoutine
(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP           Irp,
   IN PVOID          Context
);

BOOLEAN QCWT_GPSCSend(PDEVICE_EXTENSION pDevExt, UCHAR Command);

VOID QCWT_SendAndWaitGPSC(PDEVICE_EXTENSION pDevExt, UCHAR Command);

NTSTATUS QCMWT_AggregatePcaket
(
   PDEVICE_EXTENSION pDevExt,
   PVXD_WDM_IO_CONTROL_BLOCK Iob
);

VOID QCPWR_WriteAggregateDpc
(
   IN PKDPC Dpc,
   IN PVOID DeferredContext,
   IN PVOID SystemArgument1,
   IN PVOID SystemArgument2
);

#endif // QCWT_H
