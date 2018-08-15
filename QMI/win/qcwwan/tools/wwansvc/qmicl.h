// --------------------------------------------------------------------------
//
// qmicl.h
//
/// main header file
///
/// @file
//
// Copyright (C) 2013 by Qualcomm Technologies, Incorporated.
//
// All Rights Reserved.  QUALCOMM Proprietary
//
// Export of this technology or software is regulated by the U.S. Government.
// Diversion contrary to U.S. law prohibited.
//
// --------------------------------------------------------------------------
#ifndef QMICL_H
#define QMICL_H

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <string.h>
#include <winioctl.h>
#include "api.h"

#define MAX_DEC_DIGITS 10
#define QMUX_CTL_FLAG_SINGLE_MSG    0x00
#define QMUX_CTL_FLAG_COMPOUND_MSG  0x01
#define QMUX_CTL_FLAG_TYPE_CMD      0x00
#define QMUX_CTL_FLAG_TYPE_RSP      0x02
#define QMUX_CTL_FLAG_TYPE_IND      0x04
#define QMUX_CTL_FLAG_MASK_COMPOUND 0x01
#define QMUX_CTL_FLAG_MASK_TYPE     0x06 // 00-cmd, 01-rsp, 10-ind

#define QMUX_MSG_OVERHEAD_BYTES 4  // Type(USHORT) Length(USHORT)

typedef enum _QMI_RESULT_CODE_TYPE
{
   QMI_RESULT_SUCCESS = 0x0000,
   QMI_RESULT_FAILURE = 0x0001
} QMI_RESULT_CODE_TYPE;

#ifndef QC_SERVICE
#define QCMTU_Print  printf
#endif  // QC_SERVICE

typedef struct _MTU_NOTIFICATION_CONTEXT
{
   HANDLE                ServiceHandle;
   UCHAR                 IpVersion;
   NOTIFICATION_CALLBACK CallBack;
   PVOID                 Context;
} MTU_NOTIFICATION_CONTEXT, *PMTU_NOTIFICATION_CONTEXT;

// Function Prototypes

BOOL QMICL_StartMTUService(PVOID NicInfo);

HANDLE WINAPI StartMtuThread(PVOID MtuThreadContext);

DWORD WINAPI QMICL_MainThread(LPVOID Context);

int  PackWdsResetReq(VOID);

VOID UnpackWdsResetRsp(PCHAR Message, ULONG Length, PCHAR Text, ULONG TextSize);

BOOL CreateEventsForMtuThread(PDEVMN_NIC_RECORD Context);

VOID CloseEventsForMtuThread(PDEVMN_NIC_RECORD Context);

VOID MyIPV4MtuNotificationCallback
(
   HANDLE ServiceHandle,
   ULONG MtuSize,
   PDEVMN_NIC_RECORD Context
);

VOID MyIPV6MtuNotificationCallback
(
   HANDLE ServiceHandle,
   ULONG MtuSize,
   PDEVMN_NIC_RECORD Context
);

HANDLE QMICL_RegisterMtuNotification
(
   HANDLE                ServiceHandle,
   UCHAR                 IpVersion,
   NOTIFICATION_CALLBACK CallBack,
   PVOID                 Context
);

DWORD WINAPI RegisterMtuNotification(PVOID Context);

#endif // QMICL_H
