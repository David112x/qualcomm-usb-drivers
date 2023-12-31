/*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*

                             Q C S E R . H

GENERAL DESCRIPTION
   This file contains definitions for IOCTL operations.

  Copyright (c) 2014 Qualcomm Technologies, Inc.
  All rights reserved.

*====*====*====*====*====*====*====*====*====*====*====*====*====*====*====*/
/*
 * Copyright (C) 1997 by SystemSoft Corporation, as an unpublished
 * work.  All rights reserved.  Contains confidential information
 * and trade secrets proprietary to SystemSoft Corporation.
 * Disassembly or decompilation prohibited.
 *
 * $History: Sysfserioctl.h $
 * 
 * *****************  Version 1  *****************
 * User: Edk          Date: 3/27/98    Time: 10:22a
 * Created in $/Host/modem/USB WDM Driver/Generic
 *
 ************************************************************************/								    
#ifndef QCSER_H
#define QCSER_H

#include "QCMAIN.h"
						    
/*
 * Index fields...
 */
#define XMIT_IMMEDIATE_ANY	0
#define XMIT_IMMEDIATE_XOFF	1
#define XMIT_IMMEDIATE_XON	2
/*
 * Bit masks...
 */
#define DSR_BIT			0x10
#define CARRIER_DETECT_BIT	0x20
#define BREAK_DETECT		0x40
#define RING_SIGNAL		0x100

#define XON 0x11
#define XOFF 0x13
/*
 * Lengths of data fields.
 */
#define UART_STATE_SIZE        sizeof(USHORT)
#define UART_CONFIG_SIZE       7

#define MODEM_CONTEXT_BUFSIZE 16

//
// Reasons that reception may be held up.
//
#define SERIAL_RX_DTR       ((ULONG)0x01)
#define SERIAL_RX_XOFF      ((ULONG)0x02)
#define SERIAL_RX_RTS       ((ULONG)0x04)
#define SERIAL_RX_DSR       ((ULONG)0x08)

//
// Reasons that transmission may be held up.
//
#define SERIAL_TX_CTS       ((ULONG)0x01)
#define SERIAL_TX_DSR       ((ULONG)0x02)
#define SERIAL_TX_DCD       ((ULONG)0x04)
#define SERIAL_TX_XOFF      ((ULONG)0x08)
#define SERIAL_TX_BREAK     ((ULONG)0x10)

NTSTATUS InitCommStructurePointers( PDEVICE_OBJECT DeviceObject );
ULONG InitUartStateFromModem(PDEVICE_OBJECT DeviceObject, BOOLEAN bHoldWOMIrp);
NTSTATUS QCSER_Open(PVXD_WDM_IO_CONTROL_BLOCK pIOBlock);
NTSTATUS QCSER_Close(PVXD_WDM_IO_CONTROL_BLOCK pIOBlock);
NTSTATUS QCSER_StartDataThreads(PDEVICE_EXTENSION pDevExt);
NTSTATUS QCSER_StopDataThreads(PDEVICE_EXTENSION pDevExt, BOOLEAN CancelWaitWake, BOOLEAN Blocking);
VOID     QCSER_CompleteWaitOnMaskIrp(PDEVICE_EXTENSION pDevExt);
NTSTATUS SerialGetStats( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialClearStats( PDEVICE_OBJECT DeviceObject );
NTSTATUS SerialGetProperties( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialGetModemStatus( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialGetCommStatus( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialResetDevice( PDEVICE_OBJECT DeviceObject );
NTSTATUS SerialPurge( PDEVICE_OBJECT DeviceObject, PIRP pIrp );
NTSTATUS SerialLsrMstInsert( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS GetModemConfig( PDEVICE_OBJECT pDevObj, PUCHAR pBuf );
NTSTATUS SetModemConfig( PDEVICE_OBJECT pDevObj, PUCHAR pBuf );
NTSTATUS SerialGetBaudRate( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialSetBaudRate( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialSetQueueSize( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialGetHandflow( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialSetHandflow( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialGetLineControl( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialSetLineControl( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialSetBreakOn( PDEVICE_OBJECT DeviceObject );
NTSTATUS SerialSetBreakOff( PDEVICE_OBJECT DeviceObject );
NTSTATUS SerialGetTimeouts( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialSetTimeouts( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialImmediateChar( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialXoffCounter( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialSetDtr( PDEVICE_OBJECT DeviceObject );
NTSTATUS SerialClrDtr( PDEVICE_OBJECT DeviceObject );
NTSTATUS SerialSetRts( PDEVICE_OBJECT DeviceObject );
NTSTATUS SerialClrRts( PDEVICE_OBJECT DeviceObject );
NTSTATUS SerialClrDtrRts( PDEVICE_OBJECT DeviceObject, BOOLEAN bLocalOnly );
NTSTATUS SerialGetDtrRts( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialSetXon( PDEVICE_OBJECT DeviceObject );
NTSTATUS SerialSetXoff( PDEVICE_OBJECT DeviceObject );
NTSTATUS SerialGetWaitMask( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialSetWaitMask( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialWaitOnMask( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialCacheNotificationIrp( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialNotifyClient( PIRP pIrp, ULONG info );
NTSTATUS SerialGetChars( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialSetChars( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialGetDriverGUIDString( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialGetServiceKey( PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp );
NTSTATUS SerialQueryInformation(PDEVICE_OBJECT DeviceObject, PIRP pIrp);
NTSTATUS SerialSetInformation(PDEVICE_OBJECT DeviceObject, PIRP pIrp);
NTSTATUS SerialSendEncapusulatedCmd(PDEVICE_OBJECT DeviceObject, PVOID ioBuffer, PIRP pIrp);

// Windows-defined modem status register bits
// Reference: <http://msdn.microsoft.com/en-us/library/windows/hardware/ff546587(v=vs.85).aspx>

#define SERIAL_MSR_DCTS  0x01  // clr to send
#define SERIAL_MSR_DDSR  0x02  // dataset ready
#define SERIAL_MSR_TERI  0x04  // ring ind
#define SERIAL_MSR_DDCD  0x08  // carrier detect
#define SERIAL_MSR_CTS   0x10  // clr to send
#define SERIAL_MSR_DSR   0x20  // dataset ready
#define SERIAL_MSR_RI    0x40  // ring ind
#define SERIAL_MSR_DCD   0x80  // carrier detect / rcv line sig detect(RLSD)

// User-defined IOCTL code range: 2048-4095
#define QCOMSER_IOCTL_INDEX                 2048
#define QCOMSER_REMOVAL_NOTIFICATION        0x00000001L
#define QCOMSER_DUPLICATED_NOTIFICATION_REQ 0x00000002L

/*
 * IOCTLS requiring this header file.
 */

#define WIN2K_DUN_IOCTL CTL_CODE(FILE_DEVICE_SERIAL_PORT, \
               0x0FA0,          \
               METHOD_BUFFERED, \
               FILE_ANY_ACCESS)

#define IOCTL_QCOMSER_WAIT_NOTIFY CTL_CODE(FILE_DEVICE_UNKNOWN, \
                                  QCOMSER_IOCTL_INDEX+1, \
                                  METHOD_BUFFERED, \
                                  FILE_ANY_ACCESS)

/* Make the following code as 3333 */
#define IOCTL_QCOMSER_DRIVER_ID   CTL_CODE(FILE_DEVICE_UNKNOWN, \
                                  QCOMSER_IOCTL_INDEX+1285, \
                                  METHOD_BUFFERED, \
                                  FILE_ANY_ACCESS)

/* Make the following code as 3334 */
#define IOCTL_QCSER_GET_SERVICE_KEY CTL_CODE(FILE_DEVICE_UNKNOWN, \
                                  QCOMSER_IOCTL_INDEX+1286, \
                                  METHOD_BUFFERED, \
                                  FILE_ANY_ACCESS)

/* Make the following code as 3335 */
#define IOCTL_QCUSB_CDC_SEND_ENCAPSULATED CTL_CODE(FILE_DEVICE_UNKNOWN, \
                                  QCOMSER_IOCTL_INDEX+1287, \
                                  METHOD_BUFFERED, \
                                  FILE_ANY_ACCESS)

/* Make the following code as 3336 */
#define IOCTL_QCUSB_CDC_GET_ENCAPSULATED CTL_CODE(FILE_DEVICE_UNKNOWN, \
                                  QCOMSER_IOCTL_INDEX+1288, \
                                  METHOD_BUFFERED, \
                                  FILE_ANY_ACCESS)

#define IOCTL_QCDEV_REQUEST_DEVICEID CTL_CODE(FILE_DEVICE_UNKNOWN, \
                                              QCOMSER_IOCTL_INDEX+1304, \
                                              METHOD_BUFFERED, \
                                              FILE_ANY_ACCESS)

/* Make the following code as 3354 - Parent device name from filter device*/
#define IOCTL_QCDEV_GET_PARENT_DEV_NAME CTL_CODE(FILE_DEVICE_UNKNOWN, \
                                               QCOMSER_IOCTL_INDEX+1306, \
                                               METHOD_BUFFERED, \
                                               FILE_ANY_ACCESS)


#define IOCTL_QCUSB_GET_CONTROL CTL_CODE(FILE_DEVICE_UNKNOWN, \
                                  QCOMSER_IOCTL_INDEX+10, \
                                  METHOD_BUFFERED, \
                                  FILE_ANY_ACCESS)

#define IOCTL_QCUSB_SEND_CONTROL CTL_CODE(FILE_DEVICE_UNKNOWN, \
                                  QCOMSER_IOCTL_INDEX+11, \
                                  METHOD_BUFFERED, \
                                  FILE_ANY_ACCESS)

/* Make the following code as 3345 */
#define IOCTL_QCUSB_SYSTEM_POWER CTL_CODE(FILE_DEVICE_UNKNOWN, \
                                  QCOMSER_IOCTL_INDEX+20, \
                                  METHOD_BUFFERED, \
                                  FILE_ANY_ACCESS)

/* Make the following code as 3346 */
#define IOCTL_QCUSB_DEVICE_POWER CTL_CODE(FILE_DEVICE_UNKNOWN, \
                                  QCOMSER_IOCTL_INDEX+21, \
                                  METHOD_BUFFERED, \
                                  FILE_ANY_ACCESS)

/* Make the following code as 3347 */
#define IOCTL_QCUSB_QCDEV_NOTIFY CTL_CODE(FILE_DEVICE_UNKNOWN, \
                                  QCOMSER_IOCTL_INDEX+22, \
                                  METHOD_BUFFERED, \
                                  FILE_ANY_ACCESS)

/* Make the following code as 3338 - USB debug mask */
#define IOCTL_QCUSB_SET_DBG_UMSK CTL_CODE(FILE_DEVICE_UNKNOWN, \
                                  QCOMSER_IOCTL_INDEX+30, \
                                  METHOD_BUFFERED, \
                                  FILE_ANY_ACCESS)

#define IOCTL_QCUSB_REPORT_DEV_NAME CTL_CODE(FILE_DEVICE_UNKNOWN, \
                                  QCOMSER_IOCTL_INDEX+34, \
                                  METHOD_BUFFERED, \
                                  FILE_ANY_ACCESS)

#endif // QCSER_H
