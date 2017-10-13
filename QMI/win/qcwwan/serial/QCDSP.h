/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             Q C D S P . H

GENERAL DESCRIPTION
  This file contains definitions for dispatch routines.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#ifndef QCDSP_H
#define QCDSP_H

#include "QCMAIN.h"

VOID DispatchThread(PVOID pContext);
VOID DispatchCancelQueued(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS QCDSP_DirectDispatch(IN PDEVICE_OBJECT CalledDO, IN PIRP Irp);
NTSTATUS QCDSP_QueuedDispatch(IN PDEVICE_OBJECT CalledDO, IN PIRP Irp);
NTSTATUS QCDSP_Enqueue(PDEVICE_OBJECT DeviceObject, PDEVICE_OBJECT CalledDO, PIRP Irp, KIRQL Irql);
NTSTATUS InitDispatchThread(IN PDEVICE_OBJECT pDevObj);
NTSTATUS QCDSP_Dispatch
(
   IN PDEVICE_OBJECT DeviceObject,
   IN PDEVICE_OBJECT FDO,
   IN PIRP Irp,
   BOOLEAN *Removed,
   BOOLEAN ForXwdm
);
NTSTATUS QCDSP_CleanUp(IN PDEVICE_OBJECT DeviceObject, PIRP pIrp);
NTSTATUS QCDSP_SendIrpToStack(IN PDEVICE_OBJECT PortDO, IN PIRP Irp, char *info);
VOID QCDSP_PurgeDispatchQueue(PDEVICE_EXTENSION pDevExt);
NTSTATUS QCDSP_RestartDeviceFromCancelStopRemove
(
   PDEVICE_OBJECT DeviceObject,
   PIRP Irp
);
BOOLEAN QCDSP_ToProcessIrp
(
   PDEVICE_EXTENSION pDevExt,
   PIRP              Irp
);
VOID QCDSP_RequestD0(PDEVICE_EXTENSION pDevExt);
VOID QCDSP_TopD0IrpCompletionRoutine
(
   PDEVICE_OBJECT   DeviceObject,
   UCHAR            MinorFunction,
   POWER_STATE      PowerState,
   PVOID            Context,
   PIO_STATUS_BLOCK IoStatus
);

#endif // QCDSP_H
