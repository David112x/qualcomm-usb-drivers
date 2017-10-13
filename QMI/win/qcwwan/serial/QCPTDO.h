/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             Q C P T D O . H

GENERAL DESCRIPTION
   This file contains virtual COM port management functions.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#ifndef QCPTDO_H
#define QCPTDO_H

#include "QCMAIN.h"

#define QCPTDO_GetPortDOCount() gPortDOCount

PDEVICE_OBJECT QCPTDO_Create
(
   IN PDRIVER_OBJECT pDriverObject,
   IN PDEVICE_OBJECT PhysicalDeviceObject
);

// Public Interfaces
BOOLEAN QCPTDO_RemovePort(PDEVICE_OBJECT PortDO, PIRP Irp);
PDEVICE_OBJECT QCPTDO_FindPortDOByDO(PDEVICE_OBJECT PortDO, KIRQL Irql);
PDEVICE_OBJECT QCPTDO_FindPortDOByFDO(PDEVICE_OBJECT FDO, KIRQL Irql);
PDEVICE_OBJECT QCPTDO_FindPortDOByPort(PUNICODE_STRING Port);
VOID QCPTDO_DisplayListInfo(void);

// Internal Interfaces
BOOLEAN QCPTDO_ComparePortName(PUNICODE_STRING pus1, PUNICODE_STRING pus2);
VOID QCPTDO_StorePortName(char *myPortName);

#endif // QCPTDO_H
