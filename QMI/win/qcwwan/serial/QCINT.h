/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             Q C I N T . H

GENERAL DESCRIPTION
  This file contains definitions for USB interrupt operations.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
#ifndef QCINT_H
#define QCINT_H

#include "QCMAIN.h"

#define CDC_NOTIFICATION_NETWORK_CONNECTION 0x00
#define CDC_NOTIFICATION_RESPONSE_AVAILABLE 0x01
#define CDC_NOTIFICATION_SERIAL_STATE       0x20
#define CDC_NOTIFICATION_CONNECTION_SPD_CHG 0x2A

#pragma pack(push, 1)

typedef struct _USB_NOTIFICATION_STATUS
{
    UCHAR   bmRequestType;
    UCHAR   bNotification;
    USHORT  wValue;
    USHORT  wIndex;  // interface #
    USHORT  wLength; // number of data bytes
    USHORT  usValue; // serial status, etc.
} USB_NOTIFICATION_STATUS, *PUSB_NOTIFICATION_STATUS;

typedef struct _USB_NOTIFICATION_CONNECTION_SPEED
{
   ULONG ulUSBitRate;
   ULONG ulDSBitRate;
} USB_NOTIFICATION_CONNECTION_SPEED, *PUSB_NOTIFICATION_CONNECTION_SPEED;

#pragma pack(pop)

NTSTATUS QCINT_InitInterruptPipe(IN PDEVICE_OBJECT pDevObj);
VOID ReadInterruptPipe(IN PVOID pContext);
NTSTATUS InterruptPipeCompletion
(
   IN PDEVICE_OBJECT DeviceObject,
   IN PIRP           pIrp,
   IN PVOID          pContext
);
NTSTATUS CancelInterruptThread(PDEVICE_EXTENSION pDevExt, UCHAR cookie);
NTSTATUS StopInterruptService
(
   PDEVICE_EXTENSION pDevExt,
   BOOLEAN           bWait,
   BOOLEAN           CancelWaitWake,
   UCHAR             cookie
);
NTSTATUS ResumeInterruptService(PDEVICE_EXTENSION pDevExt, UCHAR cookie);

VOID QCINT_HandleSerialStateNotification(PDEVICE_EXTENSION pDevExt);
VOID QCINT_HandleNetworkConnectionNotification(PDEVICE_EXTENSION pDevExt);
VOID QCINT_HandleResponseAvailableNotification(PDEVICE_EXTENSION pDevExt);
VOID QCINT_HandleConnectionSpeedChangeNotification(PDEVICE_EXTENSION pDevExt);
VOID QCINT_StopRegistryAccess(PDEVICE_EXTENSION pDevExt);

#endif // QCINT_H
